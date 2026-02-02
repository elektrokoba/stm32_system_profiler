<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# rephrase the last task in english

Great, here's the senior-level embedded developer homework task rephrased entirely in English, using only the base ESP32 devkit hardware.

## Project Overview

Develop an **ESP32 System Profiler** that monitors system state (CPU load, heap fragmentation, task runtime statistics) under FreeRTOS, responds to interrupt-driven GPIO input (BOOT button), and streams JSON reports via UART. Goal: real-time performance analysis with <5% overhead and deep sleep optimization.

## Functional Requirements

**FreeRTOS Tasks** (5 tasks, varying priorities):

```
- profilerTask (prio 3): 100ms interval, collects FreeRTOS runtime stats, heap usage, 
  CPU load (vTaskGetRunTimeStats(), uxTaskGetStackHighWaterMark())
- gpioMonitorTask (prio 2): BOOT button (GPIO0) interrupt, dumps full system stats on press
- reportTask (prio 1): formats JSON from queue, prints to Serial (115200 baud)
- idleMonitorTask (prio 0): overrides idle hook, calculates average idle time
- watchdogTask (prio 4): feeds ESP32 TWDT, detects errors (heap<10KB, stack overflow)
```

**Hardware**: ONLY built-in peripherals!

- GPIO0 (BOOT button) - interrupt input
- UART0 (USB Serial) - JSON output
- EN pin - software deep sleep trigger


## Architecture

```
main() → FreeRTOS start → 5 tasks + 2 ISRs (GPIO0, SysTick hook)
GPIO0 IRQ → queue to gpioMonitorTask (non-blocking ISR)
profilerTask → circular buffer (100 samples) → queue → reportTask → Serial JSON
```


## JSON Output Format (1s interval)

```json
{
  "timestamp": 12345678,
  "cpu_load": 23.4,
  "heap_free": 180432,
  "heap_min": 175000,
  "frag_pct": 2.1,
  "tasks": [
    {"name":"profilerTask","runtime_pct":45.2,"stack_free":1200},
    {"name":"gpioTask","runtime_pct":1.3,"stack_free":800}
  ],
  "temp": 42.5
}
```


## Implementation Steps

1. **FreeRTOS config**: `configUSE_TRACE_FACILITY=1`, `configGENERATE_RUN_TIME_STATS=1`, custom tick hook
2. **GPIO ISR**:

```cpp
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
BaseType_t xStatus = xQueueSendFromISR(gpioQueue, &event, &xHigherPriorityTaskWoken);
```

3. **CPU Load calculation**:

```cpp
static uint32_t idleTime = 0, totalTime = 0;
void vApplicationIdleHook() { idleTime += xTaskGetTickCountFromISR(); }
cpu_load = 100.0 * (1.0 - idleTime/(float)totalTime);
```

4. **Circular buffer**: 100x system snapshot, rolling average
5. **Deep sleep**: GPIO0 long press (>3s) → `esp_deep_sleep_start()`

## Test Criteria

| Metric | Target | Measurement |
| :-- | :-- | :-- |
| Overhead | <5% CPU | FreeRTOS stats |
| Latency | <10ms IRQ→JSON | Serial timestamp diff |
| Heap | >90% free avg | uxTaskGetSystemState() |
| Stability | 24h crash-free | Watchdog counter |
| Sleep | <10uA deep sleep | Multimeter current |

## Evaluation Scale (max 100 points)

| Category | Points | Critical |
| :-- | :-- | :-- |
| FreeRTOS mastery (stats, hooks, ISR) | 30 | Context switch safe |
| JSON streaming (<1KB/s) | 20 | Non-blocking |
| GPIO IRQ handling | 20 | Debounce + queue |
| Memory profiling | 15 | Fragmentation tracking |
| Power optimization | 10 | Deep sleep wake |
| Code quality/docs | 5 | MISRA-C++ compliant |

## Submission Format

```
GitHub repo:
├── main/profiler.ino (or ESP-IDF CMakeLists.txt)
├── README.md (setup, Serial monitor screenshot, 24h log sample)
└── profiler_report.pdf (task diagram, perf charts, current consumption)
Demo video: 30s - button press → instant JSON dump
```


## Senior Bonus Tasks (+20 points)

- **Event tracing**: FreeRTOS trace buffer → binary dump decoding
- **OTA ready**: partition table config, HTTP update server
- **Factory reset**: 5x quick button presses → NVS erase

This is an **8-12 hour** senior task requiring **ONLY ESP32 devkit**, no external sensors/modules! Perfect for interviews - tests FreeRTOS expertise, ISR safety awareness, and systems-level thinking. Good luck with the implementation! 🚀

