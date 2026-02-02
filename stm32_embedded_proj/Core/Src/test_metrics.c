/**
  ******************************************************************************
  * @file    test_metrics.c
  * @brief   Test Metrics Implementation
  ******************************************************************************
  */

#include "test_metrics.h"
#include <string.h>
#include <stdio.h>

/* Test metrics storage */
static TestMetrics_t xTestMetrics = {0};
static uint32_t ulStartTime = 0;

/* For calculating running averages */
static float fCpuLoadSum = 0.0f;
static uint32_t ulHeapSum = 0;
static uint32_t ulHeapSampleCount = 0;

/**
  * @brief  Initialize test metrics tracking
  * @retval None
  */
void TestMetrics_Init(void)
{
    memset(&xTestMetrics, 0, sizeof(TestMetrics_t));
    ulStartTime = xTaskGetTickCount();
    fCpuLoadSum = 0.0f;
    ulHeapSum = 0;
    ulHeapSampleCount = 0;
    
    xTestMetrics.irqToJsonMinLatency = 0xFFFFFFFF;
    xTestMetrics.minHeapFree = 0xFFFFFFFF;
}

/**
  * @brief  Record CPU load sample
  * @param  cpuLoad: CPU load percentage (0.0-100.0)
  * @retval None
  */
void TestMetrics_RecordCpuLoad(float cpuLoad)
{
    xTestMetrics.profileCycleCount++;
    fCpuLoadSum += cpuLoad;
    xTestMetrics.avgCpuLoad = fCpuLoadSum / xTestMetrics.profileCycleCount;
    
    if (cpuLoad > xTestMetrics.maxCpuLoad) {
        xTestMetrics.maxCpuLoad = cpuLoad;
    }
    
    if (cpuLoad < xTestMetrics.minCpuLoad || xTestMetrics.minCpuLoad == 0.0f) {
        xTestMetrics.minCpuLoad = cpuLoad;
    }
}

/**
  * @brief  Record IRQ to JSON latency
  * @param  latencyMs: Latency in milliseconds
  * @retval None
  */
void TestMetrics_RecordIrqToJsonLatency(uint32_t latencyMs)
{
    xTestMetrics.jsonTransmitCount++;
    
    if (latencyMs < xTestMetrics.irqToJsonMinLatency) {
        xTestMetrics.irqToJsonMinLatency = latencyMs;
    }
    
    if (latencyMs > xTestMetrics.irqToJsonMaxLatency) {
        xTestMetrics.irqToJsonMaxLatency = latencyMs;
    }
    
    /* Running average */
    uint32_t oldAvg = xTestMetrics.irqToJsonAvgLatency;
    xTestMetrics.irqToJsonAvgLatency = 
        (oldAvg * (xTestMetrics.jsonTransmitCount - 1) + latencyMs) / xTestMetrics.jsonTransmitCount;
}

/**
  * @brief  Record heap status
  * @param  heapFree: Free heap in bytes
  * @param  fragmentation: Fragmentation percentage
  * @retval None
  */
void TestMetrics_RecordHeapStatus(uint32_t heapFree, float fragmentation)
{
    ulHeapSum += heapFree;
    ulHeapSampleCount++;
    xTestMetrics.avgHeapFree = ulHeapSum / ulHeapSampleCount;
    
    if (heapFree < xTestMetrics.minHeapFree) {
        xTestMetrics.minHeapFree = heapFree;
    }
    
    if (fragmentation > xTestMetrics.heapFragmentationMax) {
        xTestMetrics.heapFragmentationMax = fragmentation;
    }
}

/**
  * @brief  Increment watchdog feed counter
  * @retval None
  */
void TestMetrics_IncrementWatchdogFeed(void)
{
    xTestMetrics.watchdogFeedCount++;
}

/**
  * @brief  Increment stack overflow counter
  * @retval None
  */
void TestMetrics_IncrementStackOverflow(void)
{
    xTestMetrics.stackOverflowCount++;
}

/**
  * @brief  Increment malloc failure counter
  * @retval None
  */
void TestMetrics_IncrementMallocFailure(void)
{
    xTestMetrics.mallocFailureCount++;
}

/**
  * @brief  Record deep sleep event
  * @param  durationMs: Sleep duration in milliseconds
  * @param  wakeupLatencyMs: Wake-up latency in milliseconds
  * @retval None
  */
void TestMetrics_RecordDeepSleep(uint32_t durationMs, uint32_t wakeupLatencyMs)
{
    xTestMetrics.deepSleepEntryCount++;
    xTestMetrics.totalDeepSleepMs += durationMs;
    xTestMetrics.lastWakeupLatencyMs = wakeupLatencyMs;
}

/**
  * @brief  Get current metrics
  * @retval Pointer to test metrics structure
  */
TestMetrics_t* TestMetrics_GetMetrics(void)
{
    /* Update uptime */
    uint32_t ulCurrentTick = xTaskGetTickCount();
    xTestMetrics.uptimeSeconds = (ulCurrentTick - ulStartTime) / 1000;
    
    return &xTestMetrics;
}

/**
  * @brief  Get average CPU load
  * @retval Average CPU load percentage
  */
float TestMetrics_GetAverageCpuLoad(void)
{
    return xTestMetrics.avgCpuLoad;
}

/**
  * @brief  Get uptime in seconds
  * @retval Uptime in seconds
  */
uint32_t TestMetrics_GetUptimeSeconds(void)
{
    uint32_t ulCurrentTick = xTaskGetTickCount();
    return (ulCurrentTick - ulStartTime) / 1000;
}

/**
  * @brief  Check if CPU overhead is acceptable (<5%)
  * @retval 1 if acceptable, 0 if not
  */
uint8_t TestMetrics_IsCpuOverheadAcceptable(void)
{
    return (xTestMetrics.avgCpuLoad < 5.0f) ? 1 : 0;
}

/**
  * @brief  Check if heap is healthy (>90% free)
  * @retval 1 if healthy, 0 if not
  */
uint8_t TestMetrics_IsHeapHealthy(void)
{
    /* Assume total heap is 15KB for STM32 */
    uint32_t totalHeap = 15360;
    uint32_t freePercentage = (xTestMetrics.avgHeapFree * 100) / totalHeap;
    return (freePercentage > 90) ? 1 : 0;
}

/**
  * @brief  Check if latency is acceptable (<10ms)
  * @retval 1 if acceptable, 0 if not
  */
uint8_t TestMetrics_IsLatencyAcceptable(void)
{
    return (xTestMetrics.irqToJsonMaxLatency < 10) ? 1 : 0;
}

/**
  * @brief  Check if power consumption is acceptable
  * @retval 1 if acceptable (has entries deep sleep), 0 if not
  */
uint8_t TestMetrics_IsPowerConsumptionOk(void)
{
    return (xTestMetrics.deepSleepEntryCount > 0) ? 1 : 0;
}

/**
  * @brief  Print comprehensive test report
  * @retval None
  */
void TestMetrics_PrintReport(void)
{
    char buffer[512];
    int len;
    
    extern UART_HandleTypeDef huart2;
    
    len = snprintf(buffer, sizeof(buffer), 
        "\r\n========== TEST METRICS REPORT ==========\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, 1000);
    
    /* CPU Metrics */
    len = snprintf(buffer, sizeof(buffer),
        "CPU Load (avg/min/max): %.1f%% / %.1f%% / %.1f%%\r\n"
        "CPU Overhead Check: %s\r\n",
        xTestMetrics.avgCpuLoad, xTestMetrics.minCpuLoad, xTestMetrics.maxCpuLoad,
        TestMetrics_IsCpuOverheadAcceptable() ? "PASS (<5%)" : "FAIL (>5%)");
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    
    /* Latency Metrics */
    len = snprintf(buffer, sizeof(buffer),
        "IRQ→JSON Latency (min/avg/max): %lu/%lu/%lu ms\r\n"
        "Latency Check: %s\r\n",
        xTestMetrics.irqToJsonMinLatency, xTestMetrics.irqToJsonAvgLatency, xTestMetrics.irqToJsonMaxLatency,
        TestMetrics_IsLatencyAcceptable() ? "PASS (<10ms)" : "FAIL (>10ms)");
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    
    /* Heap Metrics */
    len = snprintf(buffer, sizeof(buffer),
        "Heap (avg free/min free): %lu / %lu bytes\r\n"
        "Max Fragmentation: %.1f%%\r\n"
        "Heap Health Check: %s\r\n",
        xTestMetrics.avgHeapFree, xTestMetrics.minHeapFree,
        xTestMetrics.heapFragmentationMax,
        TestMetrics_IsHeapHealthy() ? "PASS (>90% free)" : "FAIL (<90% free)");
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    
    /* Stability Metrics */
    len = snprintf(buffer, sizeof(buffer),
        "Uptime: %lu seconds (%lu hours)\r\n"
        "Watchdog Feeds: %lu\r\n"
        "Stack Overflows: %lu\r\n"
        "Malloc Failures: %lu\r\n",
        xTestMetrics.uptimeSeconds, xTestMetrics.uptimeSeconds / 3600,
        xTestMetrics.watchdogFeedCount,
        xTestMetrics.stackOverflowCount,
        xTestMetrics.mallocFailureCount);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    
    /* Sleep Metrics */
    len = snprintf(buffer, sizeof(buffer),
        "Deep Sleep Entries: %lu\r\n"
        "Total Sleep Time: %lu ms\r\n"
        "Last Wake Latency: %lu ms\r\n"
        "Power Check: %s\r\n",
        xTestMetrics.deepSleepEntryCount,
        xTestMetrics.totalDeepSleepMs,
        xTestMetrics.lastWakeupLatencyMs,
        TestMetrics_IsPowerConsumptionOk() ? "PASS (deep sleep active)" : "NOTE (no deep sleep yet)");
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
    
    /* Summary */
    uint8_t passCount = TestMetrics_IsCpuOverheadAcceptable() + 
                       TestMetrics_IsLatencyAcceptable() + 
                       TestMetrics_IsHeapHealthy() + 
                       TestMetrics_IsPowerConsumptionOk();
    
    len = snprintf(buffer, sizeof(buffer),
        "\nTest Results: %d/4 PASS\r\n"
        "========================================\r\n",
        passCount);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 1000);
}
