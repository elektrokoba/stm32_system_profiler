/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32 System Profiler - Main Application
  * @author         : Embedded Systems
  * @date           : 2026-02-02
  ******************************************************************************
  * @attention
  *
  * STM32 System Profiler with FreeRTOS
  * Monitors: CPU load, heap fragmentation, task runtime statistics
  * Features: GPIO interrupt input, UART JSON streaming, deep sleep
  * Target: <5% CPU overhead
  *
  ******************************************************************************
  */

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "system_profiler.h"
#include "json_formatter.h"
#include "test_metrics.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define PROFILER_TASK_STACK_SIZE    512
#define GPIO_MONITOR_TASK_STACK     256
#define REPORT_TASK_STACK_SIZE      512
#define IDLE_MONITOR_TASK_STACK     128
#define WATCHDOG_TASK_STACK_SIZE    256

#define PROFILER_QUEUE_LENGTH       10
#define GPIO_QUEUE_LENGTH           5

/* Deep sleep configuration */
#define BUTTON_LONG_PRESS_TIME_MS   3000  // 3 seconds for deep sleep trigger
#define BUTTON_DEBOUNCE_MS          50    // 50ms debounce

#define USER_BUTTON_PIN             GPIO_PIN_13  // Blue button on Nucleo
#define USER_BUTTON_PORT            GPIOC

#define LED_PIN                     GPIO_PIN_5   // LD2 on Nucleo
#define LED_PORT                    GPIOA

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
IWDG_HandleTypeDef hiwdg;

/* FreeRTOS Handles */
TaskHandle_t xProfilerTaskHandle = NULL;
TaskHandle_t xGpioMonitorTaskHandle = NULL;
TaskHandle_t xReportTaskHandle = NULL;
TaskHandle_t xIdleMonitorTaskHandle = NULL;
TaskHandle_t xWatchdogTaskHandle = NULL;

QueueHandle_t xProfilerQueue = NULL;
QueueHandle_t xGpioQueue = NULL;

/* Statistics tracking */
static uint32_t ulIdleCycleCount = 0;
static volatile uint32_t ulHighFrequencyTimerTicks = 0;

/* Button press tracking */
static volatile uint32_t ulButtonPressStartTime = 0;
static volatile uint8_t ucButtonPressed = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_IWDG_Init(void);
static void EnterDeepSleep(void);
static void ConfigureWakeupPin(void);

/* FreeRTOS Task Functions */
static void ProfilerTask(void *pvParameters);
static void GpioMonitorTask(void *pvParameters);
static void ReportTask(void *pvParameters);
static void IdleMonitorTask(void *pvParameters);
static void WatchdogTask(void *pvParameters);

/* Interrupt Callbacks */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/
    HAL_Init();
    SystemClock_Config();
    
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_IWDG_Init();
    
    /* Create Queues */
    xProfilerQueue = xQueueCreate(PROFILER_QUEUE_LENGTH, sizeof(SystemReport_t));
    xGpioQueue = xQueueCreate(GPIO_QUEUE_LENGTH, sizeof(uint8_t));
    
    if (xProfilerQueue == NULL || xGpioQueue == NULL) {
        Error_Handler();
    }
    
    /* Print startup message */
    char msg[] = "\r\n=== STM32 System Profiler Started ===\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    
    /* Initialize test metrics */
    TestMetrics_Init();
    
    /* Create FreeRTOS Tasks */
    xTaskCreate(ProfilerTask, "Profiler", PROFILER_TASK_STACK_SIZE, NULL, 3, &xProfilerTaskHandle);
    xTaskCreate(GpioMonitorTask, "GPIO", GPIO_MONITOR_TASK_STACK, NULL, 2, &xGpioMonitorTaskHandle);
    xTaskCreate(ReportTask, "Report", REPORT_TASK_STACK_SIZE, NULL, 1, &xReportTaskHandle);
    xTaskCreate(IdleMonitorTask, "IdleMon", IDLE_MONITOR_TASK_STACK, NULL, 0, &xIdleMonitorTaskHandle);
    xTaskCreate(WatchdogTask, "Watchdog", WATCHDOG_TASK_STACK_SIZE, NULL, 4, &xWatchdogTaskHandle);
    
    /* Start scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    while (1) {
        Error_Handler();
    }
}

/**
  * @brief  Profiler Task - Collects system statistics
  * @param  pvParameters: Task parameters
  * @retval None
  */
static void ProfilerTask(void *pvParameters)
{
    SystemReport_t report;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms interval
    static uint32_t ulButtonPressTimestamp = 0;
    
    xLastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        /* Collect system statistics */
        CollectSystemStats(&report);
        
        /* Record metrics */
        TestMetrics_RecordCpuLoad(report.cpuLoad);
        TestMetrics_RecordHeapStatus(report.heapFree, report.fragPercent);
        
        /* Send to report queue every 10 samples (1 second) */
        static uint8_t counter = 0;
        if (++counter >= 10) {
            counter = 0;
            ulButtonPressTimestamp = xTaskGetTickCount();
            xQueueSend(xProfilerQueue, &report, portMAX_DELAY);
            TestMetrics_RecordIrqToJsonLatency(0); // Latency recorded in ReportTask
        }
        
        /* Toggle LED for heartbeat */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
}

/**
  * @brief  GPIO Monitor Task - Handles button press events and long press detection
  * @param  pvParameters: Task parameters
  * @retval None
  */
static void GpioMonitorTask(void *pvParameters)
{
    uint8_t event;
    SystemReport_t report;
    uint32_t ulButtonHoldTime;
    
    for (;;) {
        /* Check for button press events every 100ms for long press detection */
        if (xQueueReceive(xGpioQueue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
            /* Button press detected - start tracking time */
            if (ucButtonPressed == 0) {
                ulButtonPressStartTime = xTaskGetTickCount();
                ucButtonPressed = 1;
                char msg[] = "\r\n=== Button Pressed (Hold for deep sleep) ===\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
            }
        }
        
        /* Monitor button hold time if pressed */
        if (ucButtonPressed) {
            ulButtonHoldTime = xTaskGetTickCount() - ulButtonPressStartTime;
            
            /* Check if button is still pressed (read GPIO directly) */
            if (HAL_GPIO_ReadPin(USER_BUTTON_PORT, USER_BUTTON_PIN) == GPIO_PIN_SET) {
                /* Button released */
                if (ulButtonHoldTime >= BUTTON_LONG_PRESS_TIME_MS) {
                    /* Long press detected - enter deep sleep */
                    char msg[] = "\r\n=== LONG PRESS DETECTED - Entering Deep Sleep ===\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    EnterDeepSleep();
                } else {
                    /* Short press - dump system stats */
                    CollectSystemStats(&report);
                    char msg[] = "\r\n=== Short Button Press - Full System Dump ===\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
                    xQueueSendToFront(xProfilerQueue, &report, 0);
                }
                
                ucButtonPressed = 0;
            }
        }
    }
}

/**
  * @brief  Report Task - Formats and transmits JSON reports
  * @param  pvParameters: Task parameters
  * @retval None
  */
static void ReportTask(void *pvParameters)
{
    SystemReport_t report;
    char jsonBuffer[1024];
    uint32_t ulReportStartTime;
    
    for (;;) {
        /* Wait for profiler data */
        if (xQueueReceive(xProfilerQueue, &report, portMAX_DELAY) == pdTRUE) {
            /* Record start time for latency measurement */
            ulReportStartTime = xTaskGetTickCount();
            
            /* Format as JSON */
            FormatSystemReportJSON(&report, jsonBuffer, sizeof(jsonBuffer));
            
            /* Transmit via UART */
            HAL_UART_Transmit(&huart2, (uint8_t*)jsonBuffer, strlen(jsonBuffer), 1000);
            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);
            
            /* Record latency from queue receive to transmission complete */
            uint32_t ulLatency = xTaskGetTickCount() - ulReportStartTime;
            TestMetrics_RecordIrqToJsonLatency(ulLatency);
        }
    }
}

/**
  * @brief  Idle Monitor Task - Tracks idle time
  * @param  pvParameters: Task parameters
  * @retval None
  */
static void IdleMonitorTask(void *pvParameters)
{
    for (;;) {
        /* Low priority task for idle monitoring */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
  * @brief  Watchdog Task - Monitors system health and feeds watchdog
  * @param  pvParameters: Task parameters
  * @retval None
  */
static void WatchdogTask(void *pvParameters)
{
    const TickType_t xFrequency = pdMS_TO_TICKS(500);
    
    for (;;) {
        vTaskDelay(xFrequency);
        
        /* Check heap status */
        size_t freeHeap = xPortGetFreeHeapSize();
        if (freeHeap < 10240) { // Less than 10KB free
            char msg[] = "WARNING: Low heap memory!\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
            TestMetrics_IncrementMallocFailure();
        }
        
        /* Feed watchdog */
        HAL_IWDG_Refresh(&hiwdg);
        TestMetrics_IncrementWatchdogFeed();
        
        /* Every 60 seconds, print test metrics */
        static uint32_t cycleCounter = 0;
        if (++cycleCounter >= 120) { // 120 * 500ms = 60 seconds
            cycleCounter = 0;
            TestMetrics_PrintReport();
        }
    }
}

/**
  * @brief  GPIO EXTI Callback - Called from interrupt with debouncing
  * @param  GPIO_Pin: Pin that triggered interrupt
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static uint32_t ulLastInterruptTime = 0;
    uint32_t ulCurrentTime = xTaskGetTickCountFromISR();
    uint8_t event = 1;
    
    if (GPIO_Pin == USER_BUTTON_PIN) {
        /* Debounce: ignore interrupts within 50ms */
        if ((ulCurrentTime - ulLastInterruptTime) > BUTTON_DEBOUNCE_MS) {
            ulLastInterruptTime = ulCurrentTime;
            /* Send event to GPIO monitor task from ISR */
            xQueueSendFromISR(xGpioQueue, &event, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/**
  * @brief  FreeRTOS Idle Hook
  * @retval None
  */
void vApplicationIdleHook(void)
{
    ulIdleCycleCount++;
}

/**
  * @brief  FreeRTOS Tick Hook for high-resolution timing
  * @retval None
  */
void vApplicationTickHook(void)
{
    ulHighFrequencyTimerTicks++;
}

/**
  * @brief  FreeRTOS Stack Overflow Hook
  * @retval None
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    char msg[64];
    snprintf(msg, sizeof(msg), "ERROR: Stack overflow in task: %s\r\n", pcTaskName);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
    TestMetrics_IncrementStackOverflow();
    Error_Handler();
}

/**
  * @brief  FreeRTOS Malloc Failed Hook
  * @retval None
  */
void vApplicationMallocFailedHook(void)
{
    char msg[] = "ERROR: Malloc failed!\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
    TestMetrics_IncrementMallocFailure();
    Error_Handler();
}

/**
  * @brief  Enter deep sleep (STM32 STOP mode)
  * @retval None (device wakes via GPIO interrupt)
  */
static void EnterDeepSleep(void)
{
    /* Disable all interrupts except EXTI for wake-up */
    __disable_irq();
    
    /* Transmit message before sleep */
    char msg[] = "Entering Stop Mode...\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
    HAL_UART_DeInit(&huart2);
    
    /* Configure GPIO for wake-up */
    ConfigureWakeupPin();
    
    /* Enable wake-up pin (GPIO13/PC13) */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    
    /* Clear wake-up flag */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    
    /* Enter STOP mode with low power regulator */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    /* Woke up - Reconfigure system clock */
    SystemClock_Config();
    
    /* Reinitialize UART */
    MX_USART2_UART_Init();
    MX_GPIO_Init();
    
    /* Re-enable interrupts */
    __enable_irq();
    
    char wakeMsg[] = "\r\n=== Woken from Deep Sleep ===\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)wakeMsg, strlen(wakeMsg), 100);
}

/**
  * @brief  Configure wake-up pin for deep sleep
  * @retval None
  */
static void ConfigureWakeupPin(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* User Button configured for wake-up */
    GPIO_InitStruct.Pin = USER_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_BUTTON_PORT, &GPIO_InitStruct);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    
    /* Initialize HSI Oscillator and activate PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    /* Initialize CPU, AHB and APB clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = 4095;
    
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    
    /* Configure LED pin as output */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    
    /* Configure User Button pin as interrupt */
    GPIO_InitStruct.Pin = USER_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_BUTTON_PORT, &GPIO_InitStruct);
    
    /* EXTI interrupt init */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(100);
    }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add implementation to report the file name and line number */
}
#endif /* USE_FULL_ASSERT */
