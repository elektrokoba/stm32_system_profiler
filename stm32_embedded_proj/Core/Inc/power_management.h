/**
  ******************************************************************************
  * @file    power_management.h
  * @brief   Power Management and Deep Sleep Configuration
  ******************************************************************************
  */

#ifndef __POWER_MANAGEMENT_H
#define __POWER_MANAGEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* Power modes */
typedef enum {
    POWER_MODE_RUN = 0,      /* Normal run mode */
    POWER_MODE_SLEEP = 1,    /* Sleep mode */
    POWER_MODE_DEEP_SLEEP = 2  /* Deep sleep (STOP mode) */
} PowerMode_t;

/* Sleep statistics */
typedef struct {
    uint32_t ulDeepSleepCount;    /* Number of times entered deep sleep */
    uint32_t ulTotalSleepTimeMs;  /* Total time spent in deep sleep */
    uint32_t ulLastWakeupTime;    /* Timestamp of last wakeup */
} SleepStats_t;

/* Function prototypes */
void PowerManagement_Init(void);
void PowerManagement_EnterDeepSleep(void);
void PowerManagement_EnterSleep(void);
PowerMode_t PowerManagement_GetCurrentMode(void);
SleepStats_t* PowerManagement_GetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_MANAGEMENT_H */
