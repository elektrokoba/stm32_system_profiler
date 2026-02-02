/**
  ******************************************************************************
  * @file    system_profiler.h
  * @brief   System Profiler Module - Collects system statistics
  ******************************************************************************
  */

#ifndef __SYSTEM_PROFILER_H
#define __SYSTEM_PROFILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* Maximum number of tasks to track */
#define MAX_TASKS                 16

/* Task statistics structure */
typedef struct {
    char taskName[16];
    float runtimePercent;
    uint32_t stackFree;
} TaskStats_t;

/* System report structure */
typedef struct {
    uint32_t timestamp;
    float cpuLoad;
    uint32_t heapFree;
    uint32_t heapMin;
    float fragPercent;
    uint8_t taskCount;
    TaskStats_t tasks[MAX_TASKS];
    float temperature;
} SystemReport_t;

/* Function prototypes */
void CollectSystemStats(SystemReport_t *report);
float CalculateCPULoad(void);
float CalculateHeapFragmentation(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_PROFILER_H */
