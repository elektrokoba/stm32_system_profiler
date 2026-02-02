/**
  ******************************************************************************
  * @file    test_metrics.h
  * @brief   Test Metrics and Performance Measurement
  ******************************************************************************
  */

#ifndef __TEST_METRICS_H
#define __TEST_METRICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "FreeRTOS.h"

/* Performance metrics structure */
typedef struct {
    /* CPU Metrics */
    float avgCpuLoad;
    float maxCpuLoad;
    float minCpuLoad;
    uint32_t profileCycleCount;
    
    /* Latency Metrics (in milliseconds) */
    uint32_t irqToJsonMinLatency;
    uint32_t irqToJsonMaxLatency;
    uint32_t irqToJsonAvgLatency;
    uint32_t jsonTransmitCount;
    
    /* Heap Metrics */
    uint32_t minHeapFree;
    uint32_t avgHeapFree;
    float heapFragmentationMax;
    
    /* Stability Metrics */
    uint32_t uptimeSeconds;
    uint32_t watchdogFeedCount;
    uint32_t stackOverflowCount;
    uint32_t mallocFailureCount;
    
    /* Sleep Metrics */
    uint32_t deepSleepEntryCount;
    uint32_t totalDeepSleepMs;
    uint32_t lastWakeupLatencyMs;
} TestMetrics_t;

/* Function prototypes */
void TestMetrics_Init(void);
void TestMetrics_RecordCpuLoad(float cpuLoad);
void TestMetrics_RecordIrqToJsonLatency(uint32_t latencyMs);
void TestMetrics_RecordHeapStatus(uint32_t heapFree, float fragmentation);
void TestMetrics_IncrementWatchdogFeed(void);
void TestMetrics_IncrementStackOverflow(void);
void TestMetrics_IncrementMallocFailure(void);
void TestMetrics_RecordDeepSleep(uint32_t durationMs, uint32_t wakeupLatencyMs);
TestMetrics_t* TestMetrics_GetMetrics(void);
void TestMetrics_PrintReport(void);
float TestMetrics_GetAverageCpuLoad(void);
uint32_t TestMetrics_GetUptimeSeconds(void);
uint8_t TestMetrics_IsCpuOverheadAcceptable(void);
uint8_t TestMetrics_IsHeapHealthy(void);
uint8_t TestMetrics_IsLatencyAcceptable(void);
uint8_t TestMetrics_IsPowerConsumptionOk(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_METRICS_H */
