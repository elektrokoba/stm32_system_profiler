# STM32 System Profiler

A real-time system profiler for STM32 microcontrollers running FreeRTOS. This project monitors system performance metrics including CPU load, heap fragmentation, task runtime statistics, and streams JSON reports via UART with <5% overhead.

## 🎯 Project Overview

**Target Platform**: STM32F4xx (Nucleo-F401RE or similar)  
**RTOS**: FreeRTOS v10.x  
**Output**: JSON over UART (115200 baud)  
**Goal**: Real-time performance analysis with minimal overhead

## ✨ Features

### System Monitoring
- **CPU Load**: Real-time CPU utilization percentage
- **Heap Management**: Free heap, minimum free heap, fragmentation analysis
- **Task Statistics**: Per-task runtime percentages and stack usage
- **Temperature**: MCU temperature monitoring (if available)

### FreeRTOS Tasks (5 tasks with varying priorities)

| Task | Priority | Period | Function |
|------|----------|--------|----------|
| **watchdogTask** | 4 (Highest) | 500ms | Feeds IWDG, monitors system health |
| **profilerTask** | 3 | 100ms | Collects FreeRTOS runtime stats |
| **gpioMonitorTask** | 2 | Event-driven | Handles button press interrupts |
| **reportTask** | 1 | Event-driven | Formats and transmits JSON reports |
| **idleMonitorTask** | 0 (Lowest) | 500ms | Tracks idle time for CPU load calculation |

### Hardware Interface
- **GPIO**: User button (PC13 on Nucleo) - interrupt-driven system dump
- **UART**: USART2 (PA2/PA3) - 115200 baud JSON output
- **LED**: LD2 (PA5) - heartbeat indicator
- **Watchdog**: IWDG for system reliability

## 📋 Requirements

### Hardware
- STM32F4 Nucleo board (or compatible STM32F4xx)
- USB cable for programming and serial communication
- No external components required

### Software
- STM32CubeIDE or STM32CubeMX + ARM toolchain
- FreeRTOS (included in STM32Cube)
- Serial terminal (PuTTY, screen, minicom, etc.)

## 🚀 Getting Started

### 1. Hardware Setup
1. Connect STM32 Nucleo board via USB
2. No additional wiring needed - uses onboard peripherals

### 2. Build and Flash

#### Using STM32CubeIDE:
```bash
# Open STM32CubeIDE
File → Import → Existing Projects into Workspace
Select this directory
Build Project (Ctrl+B)
Debug/Run (F11 or F5)
```

#### Using Command Line:
```bash
# Navigate to project directory
cd stm32_embedded_proj

# Build (assuming arm-none-eabi-gcc is installed)
make clean
make all

# Flash using ST-Link
st-flash write build/stm32_profiler.bin 0x8000000
```

### 3. Monitor Output

Connect to serial terminal:
```bash
# Linux/macOS
screen /dev/ttyACM0 115200

# Or using minicom
minicom -D /dev/ttyACM0 -b 115200

# Windows: Use PuTTY
# COM port: COMx, Baud: 115200
```

## 📊 JSON Output Format

Reports are sent every 1 second:

```json
{
  "timestamp": 12345678,
  "cpu_load": 23.4,
  "heap_free": 15360,
  "heap_min": 14800,
  "frag_pct": 2.1,
  "tasks": [
    {"name": "Profiler", "runtime_pct": 12.3, "stack_free": 1024},
    {"name": "GPIO", "runtime_pct": 1.2, "stack_free": 768},
    {"name": "Report", "runtime_pct": 8.5, "stack_free": 1200},
    {"name": "IdleMon", "runtime_pct": 0.1, "stack_free": 512},
    {"name": "Watchdog", "runtime_pct": 0.5, "stack_free": 640}
  ],
  "temp": 42.5
}
```

## 🎮 Usage

### Normal Operation
- System automatically streams JSON reports every 1 second
- LED blinks as heartbeat (100ms profiler task interval)
- Watchdog automatically feeds to prevent reset

### Button Press (User Button)
- Press blue button (PC13) for immediate full system dump
- Non-blocking interrupt-driven response
- Queue-based communication with profiler task

### Monitoring CPU Load
Monitor the `cpu_load` field to ensure profiler overhead stays <5%:
```json
"cpu_load": 4.2  // Target: <5%
```

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────┐
│                    main()                           │
│         System Init → FreeRTOS Start                │
└─────────────────────────────────────────────────────┘
                         │
        ┌────────────────┴────────────────┐
        │                                 │
   ┌────▼─────┐                    ┌─────▼────┐
   │ GPIO ISR │                    │ SysTick  │
   │  (PC13)  │                    │   Hook   │
   └────┬─────┘                    └──────────┘
        │ Queue                          │
        │                                │
   ┌────▼────────┐                       │
   │ gpioMonitor │                       │
   │    Task     │◄──────────────────────┘
   └────┬────────┘           Runtime Stats
        │                                │
        │ Trigger            ┌───────────▼─────────┐
        │                    │   profilerTask      │
        │                    │ (100ms interval)    │
        │                    └───────────┬─────────┘
        │                                │
        │ Queue             ┌────────────▼─────────┐
        └──────────────────►│   xProfilerQueue     │
                            └────────────┬─────────┘
                                         │
                            ┌────────────▼─────────┐
                            │    reportTask        │
                            │  (JSON Formatter)    │
                            └────────────┬─────────┘
                                         │
                            ┌────────────▼─────────┐
                            │    UART TX           │
                            │   (115200 baud)      │
                            └──────────────────────┘
```

## 📁 Project Structure

```
stm32_embedded_proj/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── FreeRTOSConfig.h
│   │   ├── system_profiler.h
│   │   ├── json_formatter.h
│   │   └── stm32f4xx_it.h
│   └── Src/
│       ├── main.c                    # Main application & tasks
│       ├── system_profiler.c         # Statistics collection
│       ├── json_formatter.c          # JSON serialization
│       └── stm32f4xx_it.c           # Interrupt handlers
├── Drivers/                          # STM32 HAL drivers
├── Middlewares/                      # FreeRTOS kernel
├── .ioc                             # STM32CubeMX config
└── README.md
```

## ⚙️ Configuration

### FreeRTOS Settings (FreeRTOSConfig.h)
```c
#define configUSE_TRACE_FACILITY              1
#define configGENERATE_RUN_TIME_STATS         1
#define configUSE_STATS_FORMATTING_FUNCTIONS  1
#define configUSE_IDLE_HOOK                   1
#define configUSE_TICK_HOOK                   1
```

### Memory Configuration
- **Total Heap**: 15KB (configTOTAL_HEAP_SIZE)
- **Circular Buffer**: 100 samples for statistics averaging
- **Queues**: 
  - Profiler Queue: 10 entries
  - GPIO Queue: 5 entries

## 🧪 Testing & Verification

### Performance Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| CPU Overhead | <5% | Monitor `cpu_load` in JSON output |
| IRQ Latency | <10ms | Button press → JSON dump timestamp |
| Heap Usage | >90% free avg | Monitor `heap_free` field |
| Stability | 24h crash-free | Continuous operation test |

### Test Scenarios

1. **Normal Operation Test**
   - Let system run for 5 minutes
   - Verify JSON output every second
   - CPU load should be <5%

2. **Button Response Test**
   - Press user button multiple times
   - Verify immediate JSON dump within 10ms
   - Check for debouncing behavior

3. **Memory Stress Test**
   - Monitor heap fragmentation over time
   - Should remain <3% under normal load

4. **Watchdog Test**
   - Verify watchdog refresh every 500ms
   - System should not reset during normal operation

## 🔧 Troubleshooting

### No UART Output
- Check USB cable connection
- Verify COM port in device manager
- Ensure baud rate is 115200
- Check UART2 pins (PA2/PA3)

### High CPU Load (>5%)
- Reduce profiler task frequency
- Optimize JSON formatting
- Check for stack overflow warnings

### Button Not Responding
- Verify GPIO interrupt configuration
- Check EXTI15_10_IRQn priority
- Ensure queue is not full

### System Resets
- Check watchdog feed rate
- Verify heap is not exhausted
- Look for stack overflow messages

## 📈 Advanced Features

### Circular Buffer Analysis
The system maintains a 100-sample circular buffer for rolling averages:
```c
#define STATS_BUFFER_SIZE 100
```

### CPU Load Calculation
Uses FreeRTOS runtime stats with idle task monitoring:
```c
cpu_load = 100.0 * (1.0 - idle_time / total_time)
```

### Heap Fragmentation
Tracks fragmentation based on minimum free heap:
```c
frag_pct = 100.0 * (1.0 - heap_min / heap_free)
```

## 🎓 Learning Objectives

This project demonstrates:
- ✅ **FreeRTOS Mastery**: Task scheduling, queues, hooks, runtime stats
- ✅ **Interrupt Safety**: ISR-safe queue operations, context switches
- ✅ **Memory Profiling**: Heap analysis, fragmentation tracking
- ✅ **System Architecture**: Multi-task coordination, priority design
- ✅ **Real-time Constraints**: <5% overhead, <10ms latency

## 📝 License

This project is provided as-is for educational purposes.

## 🤝 Contributing

Feel free to submit issues or pull requests for improvements!

## 📧 Contact

For questions or support, please open an issue in the repository.

---

**Happy Profiling! 🚀**
