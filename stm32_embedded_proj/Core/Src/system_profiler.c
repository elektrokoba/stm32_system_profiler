/**
  ******************************************************************************
  * @file    system_profiler.c
  * @brief   System Profiler Implementation
  ******************************************************************************
  */

#include "system_profiler.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

/* External variables */
extern volatile uint32_t ulHighFrequencyTimerTicks;

/* Static variables for CPU load calculation */
static uint32_t ulLastTotalRunTime = 0;
static uint32_t ulLastIdleRunTime = 0;

/* Circular buffer for statistics */
#define STATS_BUFFER_SIZE 100
static SystemReport_t statsBuffer[STATS_BUFFER_SIZE];
static uint8_t bufferIndex = 0;

/**
  * @brief  Collect comprehensive system statistics
  * @param  report: Pointer to SystemReport_t structure to fill
  * @retval None
  */
void CollectSystemStats(SystemReport_t *report)
{
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime, ulStatsAsPercentage;
    
    /* Get current timestamp */
    report->timestamp = xTaskGetTickCount();
    
    /* Get heap statistics */
    report->heapFree = xPortGetFreeHeapSize();
    report->heapMin = xPortGetMinimumEverFreeHeapSize();
    report->fragPercent = CalculateHeapFragmentation();
    
    /* Calculate CPU load */
    report->cpuLoad = CalculateCPULoad();
    
    /* Get number of tasks */
    uxArraySize = uxTaskGetNumberOfTasks();
    
    /* Allocate array for task status */
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        /* Generate raw status information */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        
        /* Calculate percentage for each task */
        report->taskCount = (uxArraySize < MAX_TASKS) ? uxArraySize : MAX_TASKS;
        
        for (x = 0; x < report->taskCount; x++) {
            /* Copy task name */
            strncpy(report->tasks[x].taskName, pxTaskStatusArray[x].pcTaskName, 15);
            report->tasks[x].taskName[15] = '\0';
            
            /* Calculate runtime percentage */
            ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / (ulTotalRunTime / 100);
            
            if (ulStatsAsPercentage > 0UL) {
                report->tasks[x].runtimePercent = (float)ulStatsAsPercentage;
            } else {
                report->tasks[x].runtimePercent = 0.01f;
            }
            
            /* Get stack high water mark (free stack space) */
            report->tasks[x].stackFree = pxTaskStatusArray[x].usStackHighWaterMark * sizeof(StackType_t);
        }
        
        vPortFree(pxTaskStatusArray);
    } else {
        report->taskCount = 0;
    }
    
    /* Mock temperature reading (replace with actual sensor if available) */
    report->temperature = 42.5f;
    
    /* Store in circular buffer */
    memcpy(&statsBuffer[bufferIndex], report, sizeof(SystemReport_t));
    bufferIndex = (bufferIndex + 1) % STATS_BUFFER_SIZE;
}

/**
  * @brief  Calculate CPU load percentage
  * @retval CPU load as float (0.0 - 100.0)
  */
float CalculateCPULoad(void)
{
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize;
    uint32_t ulTotalRunTime = 0;
    uint32_t ulIdleRunTime = 0;
    uint32_t ulDeltaTotal, ulDeltaIdle;
    float cpuLoad = 0.0f;
    
    /* Get number of tasks */
    uxArraySize = uxTaskGetNumberOfTasks();
    
    /* Allocate array */
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        /* Get system state */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        
        /* Find idle task runtime */
        for (UBaseType_t x = 0; x < uxArraySize; x++) {
            if (strcmp(pxTaskStatusArray[x].pcTaskName, "IDLE") == 0 ||
                strcmp(pxTaskStatusArray[x].pcTaskName, "IdleMon") == 0) {
                ulIdleRunTime += pxTaskStatusArray[x].ulRunTimeCounter;
            }
        }
        
        /* Calculate deltas */
        ulDeltaTotal = ulTotalRunTime - ulLastTotalRunTime;
        ulDeltaIdle = ulIdleRunTime - ulLastIdleRunTime;
        
        /* Calculate CPU load */
        if (ulDeltaTotal > 0) {
            cpuLoad = 100.0f * (1.0f - ((float)ulDeltaIdle / (float)ulDeltaTotal));
        }
        
        /* Update last values */
        ulLastTotalRunTime = ulTotalRunTime;
        ulLastIdleRunTime = ulIdleRunTime;
        
        vPortFree(pxTaskStatusArray);
    }
    
    /* Clamp between 0 and 100 */
    if (cpuLoad < 0.0f) cpuLoad = 0.0f;
    if (cpuLoad > 100.0f) cpuLoad = 100.0f;
    
    return cpuLoad;
}

/**
  * @brief  Calculate heap fragmentation percentage
  * @retval Fragmentation percentage as float
  */
float CalculateHeapFragmentation(void)
{
    size_t freeHeap = xPortGetFreeHeapSize();
    size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize();
    float fragPercent = 0.0f;
    
    if (freeHeap > 0) {
        fragPercent = 100.0f * (1.0f - ((float)minFreeHeap / (float)freeHeap));
    }
    
    return fragPercent;
}

/**
  * @brief  Get statistics from circular buffer
  * @param  index: Index in circular buffer (0 = most recent)
  * @param  report: Pointer to output report
  * @retval 1 if successful, 0 if invalid index
  */
uint8_t GetBufferedStats(uint8_t index, SystemReport_t *report)
{
    if (index >= STATS_BUFFER_SIZE) {
        return 0;
    }
    
    uint8_t actualIndex = (bufferIndex - 1 - index + STATS_BUFFER_SIZE) % STATS_BUFFER_SIZE;
    memcpy(report, &statsBuffer[actualIndex], sizeof(SystemReport_t));
    
    return 1;
}
