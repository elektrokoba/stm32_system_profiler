# Test Criteria Implementation Guide

## Overview
This document describes how to measure and verify all test criteria for the STM32 System Profiler project.

## Test Metrics Module

A comprehensive `test_metrics` module has been integrated to track all performance indicators.

### Metrics Available

#### 1. CPU Overhead (<5% target)
```c
float avgCpuLoad = TestMetrics_GetAverageCpuLoad();
uint8_t passed = TestMetrics_IsCpuOverheadAcceptable();  // Returns 1 if <5%
```

**Measurement Method:**
- Collected from FreeRTOS runtime statistics every 100ms
- Calculated as: `1.0 - (idle_ticks / total_ticks) * 100`
- Running average maintained across all profiler cycles
- Displayed in every test report

**Expected Output:**
```json
CPU Load (avg/min/max): 3.2% / 1.5% / 4.8%
CPU Overhead Check: PASS (<5%)
```

---

#### 2. Latency (<10ms IRQ→JSON target)
```c
uint32_t maxLatency = xTestMetrics.irqToJsonMaxLatency;
uint8_t passed = TestMetrics_IsLatencyAcceptable();  // Returns 1 if <10ms
```

**Measurement Method:**
- Timestamp recorded when JSON queue receive event begins
- Timestamp recorded when UART transmission completes
- Difference calculated as latency in milliseconds
- Min/max/average tracked across all JSON transmissions

**Expected Output:**
```json
IRQ→JSON Latency (min/avg/max): 2/5/9 ms
Latency Check: PASS (<10ms)
```

**Contributing Factors:**
- GPIO ISR execution: ~1ms
- Queue send + context switch: ~2-3ms
- JSON formatting: ~2ms
- UART transmission (varies by size): ~1-3ms
- **Total typical: 5-8ms**

---

#### 3. Heap Health (>90% free average)
```c
uint32_t avgHeapFree = xTestMetrics.avgHeapFree;
float maxFragmentation = xTestMetrics.heapFragmentationMax;
uint8_t passed = TestMetrics_IsHeapHealthy();  // Returns 1 if >90% free
```

**Measurement Method:**
- Free heap sampled every 100ms via `xPortGetFreeHeapSize()`
- Minimum free heap tracked via `xPortGetMinimumEverFreeHeapSize()`
- Fragmentation calculated as: `100.0 * (1.0 - min_free / current_free)`
- Running average maintains heap health trend

**Expected Output:**
```json
Heap (avg free/min free): 13850 / 13200 bytes
Max Fragmentation: 4.7%
Heap Health Check: PASS (>90% free)
```

**Configuration:**
- Total heap: 15360 bytes (configTOTAL_HEAP_SIZE)
- System allocation threshold: 1.5KB for tasks & queues
- Available for profiler data: ~13.8KB

---

#### 4. Stability (24h crash-free target)
```c
uint32_t uptimeSeconds = TestMetrics_GetUptimeSeconds();
uint32_t watchdogFeeds = xTestMetrics.watchdogFeedCount;
uint32_t stackOverflows = xTestMetrics.stackOverflowCount;
uint32_t mallocFailures = xTestMetrics.mallocFailureCount;
```

**Measurement Method:**
- Uptime tracked from system start
- Watchdog feed incremented every 500ms (should be steady)
- Stack overflow counter incremented on vApplicationStackOverflowHook()
- Malloc failures tracked on vApplicationMallocFailedHook()

**Expected Output (after 24h):**
```json
Uptime: 86400 seconds (24 hours)
Watchdog Feeds: 172800 (every 500ms = 2 feeds/sec)
Stack Overflows: 0
Malloc Failures: 0
```

**Verification Calculations:**
- Expected watchdog feeds = 86400 / 0.5 = 172,800 ✓
- No crashes/restarts (watchdog feeds increase monotonically)
- No memory exhaustion events

---

#### 5. Sleep Mode Power (<10µA target)
```c
uint32_t deepSleepEntries = xTestMetrics.deepSleepEntryCount;
uint32_t totalSleepMs = xTestMetrics.totalDeepSleepMs;
uint32_t lastWakeLatency = xTestMetrics.lastWakeupLatencyMs;
uint8_t passed = TestMetrics_IsPowerConsumptionOk();
```

**Measurement Method:**
- Deep sleep entry counter incremented when entering STOP mode
- Total sleep duration accumulated in milliseconds
- Wake-up latency measured from GPIO interrupt to code execution

**Expected Output:**
```json
Deep Sleep Entries: 5
Total Sleep Time: 45000 ms
Last Wake Latency: 15 ms
Power Check: PASS (deep sleep active)
```

**Power Consumption Breakdown:**
- Run mode: ~50-100mA
- Sleep mode (WFI): ~30-50mA
- Stop mode (STOP2): <10µA (target)
- Typical duty cycle at 1s reports: ~99.9% in STOP mode

---

## Running Test Metrics

### Automatic Test Report (every 60 seconds)

The watchdog task automatically prints a comprehensive report every 60 seconds:

```
========== TEST METRICS REPORT ==========
CPU Load (avg/min/max): 3.2% / 1.5% / 4.8%
CPU Overhead Check: PASS (<5%)
IRQ→JSON Latency (min/avg/max): 2/5/9 ms
Latency Check: PASS (<10ms)
Heap (avg free/min free): 13850 / 13200 bytes
Max Fragmentation: 4.7%
Heap Health Check: PASS (>90% free)
Uptime: 60 seconds (0 hours)
Watchdog Feeds: 120
Stack Overflows: 0
Malloc Failures: 0
Deep Sleep Entries: 1
Total Sleep Time: 3000 ms
Last Wake Latency: 15 ms
Power Check: PASS (deep sleep active)

Test Results: 4/4 PASS
========================================
```

### Manual Metric Query

Access metrics at any time:
```c
TestMetrics_t *metrics = TestMetrics_GetMetrics();
printf("Current CPU Load: %.1f%%\n", metrics->avgCpuLoad);
printf("Uptime: %lu seconds\n", metrics->uptimeSeconds);
```

---

## 24-Hour Stability Test Procedure

### Setup
1. Flash STM32 with profiler firmware
2. Connect to serial terminal (115200 baud)
3. Place device under constant load (button presses every 5-10 minutes)
4. Monitor watchdog feed count and error messages

### Expected Behavior
- JSON reports every 1 second (300+ per hour)
- Watchdog feeds every 500ms (172,800 in 24 hours)
- No stack overflow or malloc failure messages
- CPU load remains <5%
- Heap maintains >90% free average

### Failure Indicators
- Watchdog feed count gaps → System reset occurred
- Stack overflow message → Task exceeded allocated stack
- Malloc failure → Out of memory condition
- CPU load >5% → Profiler overhead too high
- Heap <90% free → Memory leak detected

### Success Criteria (all must be true)
```
✓ Uptime ≥ 86400 seconds
✓ Watchdog feeds ≥ 172,400 (97%+ success rate)
✓ Stack overflows = 0
✓ Malloc failures = 0
✓ Avg CPU load < 5%
✓ Avg heap free > 90%
✓ Latency max < 10ms
```

---

## Deep Sleep Verification

### Test Procedure

1. **Press button for <3 seconds**
   - Expect: Quick JSON dump (short latency)
   - Check: LED blinks, serial output appears

2. **Hold button for ≥3 seconds**
   - Expect: "Entering Deep Sleep" message
   - Check: Device enters STOP mode
   - LED stops blinking (no CPU activity)

3. **Press button again to wake**
   - Expect: "Woken from Deep Sleep" message
   - Measured wake-up latency in test report

4. **Measure current consumption**
   ```
   Multimeter setup:
   - Set to DC current (mA or µA range)
   - Connect in series with power supply
   
   Readings:
   - Active: 50-100 mA
   - Stop mode: <10 µA target
   ```

---

## JSON Output with Metrics

Each 1-second JSON report now includes timing data:
```json
{
  "timestamp": 12345,
  "cpu_load": 3.2,
  "heap_free": 13850,
  "heap_min": 13200,
  "frag_pct": 4.7,
  "tasks": [
    {"name":"Profiler","runtime_pct":2.1,"stack_free":1024},
    {"name":"Report","runtime_pct":0.8,"stack_free":1200}
  ],
  "temp": 42.5
}
```

---

## Evaluation Checklist

| Metric | Target | Method | Pass/Fail |
|--------|--------|--------|-----------|
| CPU Overhead | <5% | FreeRTOS stats (auto-reported) | ✓/✗ |
| Latency | <10ms | IRQ→JSON timestamp (auto-tracked) | ✓/✗ |
| Heap Health | >90% free | Heap sampling (auto-tracked) | ✓/✗ |
| Stability | 24h crash-free | Uptime counter + watchdog feeds | ✓/✗ |
| Sleep Mode | <10µA | Current measurement + entry count | ✓/✗ |

---

## Troubleshooting Test Failures

### High CPU Load (>5%)
- **Cause**: Profiler task running too frequently
- **Solution**: Reduce profiler frequency or JSON transmission rate
- **Check**: `PROFILER_QUEUE_LENGTH` configuration

### High Latency (>10ms)
- **Cause**: UART transmission delay or queue contention
- **Solution**: Increase UART baud rate or reduce JSON size
- **Check**: `FormatSystemReportJSON()` performance

### Low Heap (<90%)
- **Cause**: Memory leak or excessive allocation
- **Solution**: Check task allocations, reduce buffer sizes
- **Check**: `xPortGetMinimumEverFreeHeapSize()` trend

### Stability Issues
- **Cause**: Stack overflow or watchdog timeout
- **Solution**: Increase task stack sizes or reduce profiler load
- **Check**: Watchdog feed consistency

### Sleep Mode Not Working
- **Cause**: GPIO configuration or wake-up pin issue
- **Solution**: Verify GPIO setup and interrupt configuration
- **Check**: `ConfigureWakeupPin()` and `EnterDeepSleep()`

---

## Performance Tuning

### To Reduce CPU Overhead
```c
#define PROFILER_QUEUE_LENGTH       5   // Reduce from 10
// Profiler will send reports less frequently
```

### To Improve Latency
```c
#define UART_BAUD_RATE              921600  // Increase from 115200
// UART transmissions complete faster
```

### To Optimize Heap
```c
#define MAX_TASKS                   8   // Track fewer tasks
// Reduces SystemReport_t size
```

---

Generated: 2026-02-02
