/**
  ******************************************************************************
  * @file    power_management.c
  * @brief   Power Management Implementation
  ******************************************************************************
  */

#include "power_management.h"
#include <string.h>

/* Static variables */
static PowerMode_t xCurrentPowerMode = POWER_MODE_RUN;
static SleepStats_t xSleepStats = {0};

/**
  * @brief  Initialize power management
  * @retval None
  */
void PowerManagement_Init(void)
{
    /* Enable PWR clock */
    __HAL_RCC_PWR_CLK_ENABLE();
    
    /* Configure power regulator and system clock behavior in STOP mode */
    HAL_PWR_EnableBkUpAccess();
    
    /* Initialize statistics */
    memset(&xSleepStats, 0, sizeof(SleepStats_t));
    xCurrentPowerMode = POWER_MODE_RUN;
}

/**
  * @brief  Enter deep sleep (STOP) mode
  * @retval None
  */
void PowerManagement_EnterDeepSleep(void)
{
    uint32_t ulSleepStartTime = HAL_GetTick();
    
    xCurrentPowerMode = POWER_MODE_DEEP_SLEEP;
    
    /* Ensure PLL is disabled before STOP mode */
    __HAL_RCC_PLL_DISABLE();
    
    /* Disable systick interrupt to reduce power consumption */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    
    /* Clear all EXTI pending bits */
    EXTI->PR = 0xFFFFFFFF;
    
    /* Enter STOP mode with low power regulator */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    /* Woke up from STOP mode */
    
    /* Re-enable systick */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    
    /* Update sleep statistics */
    uint32_t ulSleepDuration = HAL_GetTick() - ulSleepStartTime;
    xSleepStats.ulDeepSleepCount++;
    xSleepStats.ulTotalSleepTimeMs += ulSleepDuration;
    xSleepStats.ulLastWakeupTime = HAL_GetTick();
    
    xCurrentPowerMode = POWER_MODE_RUN;
}

/**
  * @brief  Enter sleep mode (WFI)
  * @retval None
  */
void PowerManagement_EnterSleep(void)
{
    xCurrentPowerMode = POWER_MODE_SLEEP;
    
    /* Enter sleep mode - CPU stops but clocks continue */
    __WFI();
    
    xCurrentPowerMode = POWER_MODE_RUN;
}

/**
  * @brief  Get current power mode
  * @retval Current power mode
  */
PowerMode_t PowerManagement_GetCurrentMode(void)
{
    return xCurrentPowerMode;
}

/**
  * @brief  Get sleep statistics
  * @retval Pointer to sleep statistics structure
  */
SleepStats_t* PowerManagement_GetStats(void)
{
    return &xSleepStats;
}
