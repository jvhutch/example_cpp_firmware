CROSS_COMPILE ?= arm-none-eabi-

CXX := $(CROSS_COMPILE)g++
CC  := $(CROSS_COMPILE)gcc
LD  := $(CROSS_COMPILE)g++
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size

TARGET := firmware
BUILD_DIR ?= build

TARGET_ELF := $(BUILD_DIR)/$(TARGET).elf
TARGET_BIN := $(BUILD_DIR)/$(TARGET).bin

CPUFLAGS := -mcpu=cortex-a15 -marm

CXXFLAGS := \
	$(CPUFLAGS) \
	-std=c++17 \
	-ffreestanding \
	-fno-exceptions \
	-fno-rtti \
	-fno-use-cxa-atexit \
	-fno-threadsafe-statics \
	-fno-stack-protector \
	-ffunction-sections \
	-fdata-sections \
	-Wall \
	-Wextra \
	-O2

ASFLAGS := \
	$(CPUFLAGS) \
	-ffreestanding

LDFLAGS := \
	$(CPUFLAGS) \
	-T linker.ld \
	-nostdlib \
	-Wl,--gc-sections \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map

LDLIBS := -lgcc

OBJS := $(BUILD_DIR)/startup.o $(BUILD_DIR)/main.o

all: $(TARGET_ELF) $(TARGET_BIN) size

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/startup.o: startup.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET_ELF): $(OBJS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

size: $(TARGET_ELF)
	$(SIZE) $<

run: $(TARGET_ELF)
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $(TARGET_ELF)

debug: $(TARGET_ELF)
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $(TARGET_ELF) \
		-S -s

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run debug clean size
