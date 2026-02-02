# STM32 System Profiler Makefile
# Minimal example - adapt for your toolchain

# Project name
PROJECT = stm32_profiler

# Target
TARGET = STM32F401xE

# Toolchain
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# Directories
BUILD_DIR = build
SRC_DIR = Core/Src
INC_DIR = Core/Inc

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Include paths
INCLUDES = -I$(INC_DIR) \
           -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
           -IDrivers/CMSIS/Include \
           -IMiddlewares/Third_Party/FreeRTOS/Source/include \
           -IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F

# Compiler flags
CFLAGS = -mcpu=cortex-m4 \
         -mthumb \
         -mfloat-abi=hard \
         -mfpu=fpv4-sp-d16 \
         -O2 \
         -g \
         -Wall \
         $(INCLUDES) \
         -D$(TARGET)

# Linker flags
LDFLAGS = -mcpu=cortex-m4 \
          -mthumb \
          -mfloat-abi=hard \
          -mfpu=fpv4-sp-d16 \
          -specs=nosys.specs \
          -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map \
          -Wl,--gc-sections

# Default target
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link
$(BUILD_DIR)/$(PROJECT).elf: $(BUILD_DIR)
	@echo "Note: This is a minimal Makefile example."
	@echo "For a complete build, use STM32CubeIDE or generate full Makefile with STM32CubeMX."

# Binary
$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $<

# Clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
