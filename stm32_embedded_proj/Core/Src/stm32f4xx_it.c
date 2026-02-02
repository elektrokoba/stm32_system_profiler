/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines
  ******************************************************************************
  */

#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"
#include "main.h"

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart2;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
    while (1) {
    }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
    while (1) {
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
    while (1) {
    }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
    while (1) {
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
    while (1) {
    }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
