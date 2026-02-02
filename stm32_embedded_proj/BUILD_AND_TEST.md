# How to Build and Test the STM32 System Profiler

## Prerequisites

### Hardware Required
- **STM32F401RE Nucleo Board** (or compatible STM32F4xx)
- **USB Cable** (USB-A to Micro-USB)
- **Serial Terminal** (PuTTY, screen, minicom, or Arduino IDE Serial Monitor)
- **Optional:** Multimeter (for power consumption measurement)

### Software Required
- **STM32CubeIDE** (free download from st.com)
- **STM32CubeMX** (for peripheral configuration)
- **ARM GCC Toolchain** (usually included with STM32CubeIDE)
- **Python 3.x** (for test report generation - optional)

---

## Building the Project

### Method 1: STM32CubeIDE (Recommended)

```bash
# 1. Clone/download the project
cd /Users/macbook/work/stm32_embedded_proj

# 2. Open STM32CubeIDE
# File → Open Projects from File System
# Select: stm32_embedded_proj

# 3. Clean and Build
# Right-click project → Clean
# Right-click project → Build Project

# 4. Expected Output:
#    [100%] Linking target stm32_profiler.elf
#    text    data     bss     dec     hex filename
#    45832    820    8892   55544    d8e8 stm32_profiler.elf
#    Build completed successfully
```

### Method 2: Command Line (Make)

```bash
# Requires arm-none-eabi-gcc and STM32 HAL drivers configured
cd stm32_embedded_proj
make clean
make all -j4

# Binary output: build/stm32_profiler.bin
```

---

## Flashing the Firmware

### Method 1: STM32CubeIDE

```
# 1. Connect Nucleo board via USB
# 2. In STM32CubeIDE: Run → Run As → STM32 MCU C/C++ Application
# 3. Firmware automatically flashes and boots
```

### Method 2: STLink Command Line

```bash
# Connect board via USB, then:
st-flash write build/stm32_profiler.bin 0x8000000

# Expected output:
# st-flash 1.7.0
# 2022-02-02T10:30:45 INFO common.c: Loading device parameters....
# 2022-02-02T10:30:45 INFO common.c: Device connected is: STM32F401xx...
# 2022-02-02T10:30:45 INFO common.c: SRAM size: 0x10000 bytes (64 KiB)
# 2022-02-02T10:30:46 INFO common.c: Flash written and verified!
```

### Method 3: OpenOCD

```bash
openocd -f board/st_nucleo_f4.cfg -c "program build/stm32_profiler.bin verify reset exit 0x08000000"
```

---

## Running Tests

### 1. Connect Serial Terminal

```bash
# Linux/macOS
screen /dev/ttyACM0 115200

# Or with minicom
minicom -D /dev/ttyACM0 -b 115200

# Windows (PuTTY)
# COM port: COMx (check Device Manager)
# Baud rate: 115200
```

### 2. Expected Startup Output

```
=== STM32 System Profiler Started ===
Profiler initialized...
FreeRTOS Scheduler running...

{
  "timestamp": 1000,
  "cpu_load": 2.3,
  "heap_free": 14250,
  "heap_min": 13200,
  "frag_pct": 1.8,
  "tasks": [
    {"name":"Profiler","runtime_pct":1.2,"stack_free":1024},
    {"name":"GPIO","runtime_pct":0.2,"stack_free":768},
    {"name":"Report","runtime_pct":0.7,"stack_free":1200},
    {"name":"IdleMon","runtime_pct":0.0,"stack_free":512},
    {"name":"Watchdog","runtime_pct":0.2,"stack_free":640}
  ],
  "temp": 42.5
}
```

---

## Test Procedures

### Test 1: CPU Overhead Measurement

**Goal:** Verify <5% CPU load

**Procedure:**
```
1. Let system run for 5 minutes
2. Monitor JSON output for "cpu_load" field
3. Collect at least 300 samples
4. Calculate average
```

**Expected Result:**
```
Sample Results (5 minutes):
  Min CPU:    1.2%
  Avg CPU:    3.2%  ← Should be <5% ✓
  Max CPU:    4.8%
```

**Pass Criteria:** `avg_cpu_load < 5.0%`

---

### Test 2: Latency Measurement

**Goal:** Verify <10ms IRQ→JSON latency

**Procedure:**
```
1. Open two terminal windows
2. Terminal 1: Watch JSON timestamps
3. Terminal 2: Record button press time
4. Press USER BUTTON on Nucleo board
5. Note timestamp difference

Calculation:
  JSON timestamp (ms) - Button press time (ms) = Latency
```

**Expected Result:**
```
Button Press 1: 5.2ms latency ✓
Button Press 2: 7.8ms latency ✓
Button Press 3: 4.1ms latency ✓
Button Press 4: 6.5ms latency ✓
Button Press 5: 8.0ms latency ✓

Max Latency: 8.0ms (target <10ms) ✓ PASS
```

**Pass Criteria:** `max_latency < 10ms`

---

### Test 3: Heap Health Check

**Goal:** Verify >90% free heap average

**Procedure:**
```
1. Collect 100 JSON reports (100 seconds)
2. Extract "heap_free" and "heap_min" fields
3. Calculate percentage: (heap_free / 15360) * 100
```

**Expected Result:**
```
Sample Heap Data:
  Report  1: 14150/15360 = 92.1%
  Report  2: 14100/15360 = 91.8%
  Report  3: 14200/15360 = 92.5%
  ...
  Report 100: 14120/15360 = 91.9%

Average: 92.1% (target >90%) ✓ PASS
```

**Pass Criteria:** `avg_heap_percent > 90%`

---

### Test 4: Stability Check (24-hour test)

**Goal:** Verify crash-free operation for 24 hours

**Procedure:**
```
1. Start system with test metrics enabled
2. Let run for 24 hours continuously
3. Monitor for errors
4. Check periodic test reports (every 60 seconds)
```

**Expected Result:**
```
========== TEST METRICS REPORT ==========
Uptime: 86400 seconds (24 hours)
Watchdog Feeds: 172800 (expected 172800) ✓
Stack Overflows: 0 ✓
Malloc Failures: 0 ✓
...
Test Results: 4/4 PASS ✓
========================================
```

**Pass Criteria:** 
- `uptime >= 86400 seconds`
- `watchdog_feeds >= 172400`
- `errors == 0`

---

### Test 5: Deep Sleep Power Test

**Goal:** Verify <10µA sleep current

**Equipment:**
```
Multimeter or Power Analyzer configured for:
  - DC Current measurement
  - Range: µA
  - Connected in series with 5V supply
```

**Procedure:**
```
1. Device running in normal mode (72mA baseline)
2. Press USER BUTTON and hold for >3 seconds
3. Note "Entering Deep Sleep" message
4. Watch multimeter reading drop
5. Record current value (should be <10µA)
6. Press button again to wake
7. Verify "Woken from Deep Sleep" message
```

**Expected Result:**
```
Button Press: Entering Deep Sleep...
  Run Mode Current:   72.3 mA
  Sleep Mode Current:  8.2 µA  ← Target <10µA ✓
  
Multimeter Reading:   8.2 µA (target <10µA) ✓ PASS
```

**Pass Criteria:** `sleep_current < 10µA`

---

## Automated Test Report Generation

### Every 60 Seconds

The system automatically prints a test report:

```bash
# Wait for this output in serial terminal:

========== TEST METRICS REPORT ==========
CPU Load (avg/min/max): 3.2% / 1.2% / 4.8%
CPU Overhead Check: PASS (<5%)
IRQ→JSON Latency (min/avg/max): 2/5/8 ms
Latency Check: PASS (<10ms)
Heap (avg free/min free): 14150 / 13230 bytes
Max Fragmentation: 2.1%
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

---

## Troubleshooting

### No Serial Output

**Symptoms:** Terminal shows no data

**Solutions:**
```
1. Check USB cable connection
2. Verify correct COM port (Device Manager)
3. Confirm baud rate is 115200
4. Check if board needs to be reset:
   - Press RESET button on Nucleo
   - Or send Ctrl+C in terminal
5. Verify UART2 jumpers (PA2/PA3) configured
```

### High CPU Load (>5%)

**Symptoms:** CPU load field shows >5%

**Solutions:**
```
1. Reduce profiler frequency:
   #define PROFILER_TASK_STACK_SIZE 256
2. Decrease profiler queue size:
   #define PROFILER_QUEUE_LENGTH 5
3. Reduce JSON transmission frequency:
   if (++counter >= 20)  // Instead of 10
```

### Low Heap (<90%)

**Symptoms:** Heap percentage drops below 90%

**Solutions:**
```
1. Reduce circular buffer size:
   #define STATS_BUFFER_SIZE 50
2. Reduce max tasks tracked:
   #define MAX_TASKS 8
3. Check for memory leaks:
   - Watch heap_min field
   - Should increase slowly or stay flat
```

### Deep Sleep Not Working

**Symptoms:** "Entering Deep Sleep" message but current doesn't drop

**Solutions:**
```
1. Verify GPIO wake-up pin configuration
2. Check EXTI interrupt setup
3. Ensure PWR clock is enabled:
   __HAL_RCC_PWR_CLK_ENABLE();
4. Test with longer press (>3 seconds)
```

---

## Performance Tuning

### Reduce Power Consumption Further

```c
// In main.c, profiler task:
#define PROFILER_FREQUENCY_MS 500  // Increase from 100ms
// Reports less frequently → Lower CPU load
```

### Improve Latency

```c
// In stm32f4xx.c, increase UART speed:
huart2.Init.BaudRate = 921600;  // Instead of 115200
// Faster transmission = Lower latency
```

### Optimize Memory

```c
// In system_profiler.h, reduce task count:
#define MAX_TASKS 8  // Instead of 16
// Smaller JSON → More memory available
```

---

## Collecting Test Data

### For Analysis

```bash
# Capture 1000 JSON reports to file
# Terminal 1: Pipe output to file
screen /dev/ttyACM0 115200 -L -Logfile profiler_data.log

# Run for 1000 seconds (~16 minutes)
# Stop with Ctrl+C
# Parse with custom script
```

### Python Data Analysis Example

```python
import json
import re

with open('profiler_data.log', 'r') as f:
    data = f.readlines()

cpu_loads = []
for line in data:
    if '"cpu_load"' in line:
        match = re.search(r'"cpu_load":\s*([\d.]+)', line)
        if match:
            cpu_loads.append(float(match.group(1)))

print(f"Avg CPU: {sum(cpu_loads)/len(cpu_loads):.1f}%")
print(f"Max CPU: {max(cpu_loads):.1f}%")
print(f"Min CPU: {min(cpu_loads):.1f}%")
```

---

## Expected Test Duration

| Test | Duration | Notes |
|------|----------|-------|
| CPU Overhead | 5 min | Quick baseline |
| Latency | 10 min | 50 button presses |
| Heap Health | 5 min | 300 samples |
| Stability (short) | 1 hour | Sanity check |
| **Stability (full)** | **24 hours** | Full certification |
| Deep Sleep | 30 min | Power measurement |

**Total Time for Full Certification: 24+ hours**

---

## Success Criteria Summary

```
✓ CPU Overhead:      <5.0%      (Target: PASS if <5%)
✓ IRQ→JSON Latency:  <10ms      (Target: PASS if <10ms)  
✓ Heap Health:       >90% free  (Target: PASS if >90%)
✓ Stability:         0 errors   (Target: PASS if stable)
✓ Deep Sleep:        <10µA      (Target: PASS if <10µA)

If all ✓ PASS → System is PRODUCTION READY
```

---

## Next Steps

1. **Build:** `make all`
2. **Flash:** `st-flash write build/stm32_profiler.bin 0x8000000`
3. **Test:** Follow procedures above
4. **Validate:** All criteria must PASS
5. **Deploy:** Production ready!

Happy Testing! 🚀

---

**Last Updated:** 2026-02-02  
**Version:** 1.0.0
