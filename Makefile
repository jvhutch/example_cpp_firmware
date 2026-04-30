CROSS_COMPILE ?= arm-none-eabi-

CXX := $(CROSS_COMPILE)g++
CC  := $(CROSS_COMPILE)gcc
LD  := $(CROSS_COMPILE)g++
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size

TARGET := firmware
# BUILD_DIR ?= build

# Detect operating system
UNAME_S := $(shell uname -s)

# Default build directory
BUILD_ROOT		?= $(PWD)/build
BUILD_DIR		?= $(BUILD_ROOT)/artifacts
RAM_DISK_SIZE		?= 4M

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

DEBUG_CXXFLAGS := $(filter-out -O2,$(CXXFLAGS)) -Og -g3 -fno-omit-frame-pointer
DEBUG_ASFLAGS = $(ASFLAGS) -g3

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
GDB := $(or \
	$(shell command -v arm-none-eabi-gdb 2>/dev/null), \
	$(shell command -v gdb-multiarch 2>/dev/null), \
	$(shell command -v gdb 2>/dev/null))

OBJS := $(BUILD_DIR)/startup.o $(BUILD_DIR)/main.o

all: $(TARGET_ELF) $(TARGET_BIN) size

# $(BUILD_DIR):
# 	mkdir -p $@

$(BUILD_DIR): $(MAKEFILE_LIST)
	mkdir -p $(BUILD_ROOT)
ifeq ($(UNAME_S),Darwin)
	mount -t tmpfs | grep -q $(BUILD_ROOT) || sudo mount_tmpfs -s $(RAM_DISK_SIZE) $(BUILD_ROOT)
	touch $(BUILD_ROOT)/.metadata_never_index
endif
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/startup.o: startup.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/startup.debug.o: startup.S | $(BUILD_DIR)
	$(CC) $(DEBUG_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/main.debug.o: main.cpp | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

DEBUG_OBJS := $(BUILD_DIR)/startup.debug.o $(BUILD_DIR)/main.debug.o

$(BUILD_DIR)/$(TARGET).debug.elf: $(DEBUG_OBJS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(DEBUG_OBJS) $(LDLIBS) -o $@

$(BUILD_DIR)/$(TARGET).debug.bin: $(BUILD_DIR)/$(TARGET).debug.elf
	$(OBJCOPY) -O binary $< $@

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

debug-build: $(BUILD_DIR)/$(TARGET).debug.elf $(BUILD_DIR)/$(TARGET).debug.bin

debug-run: $(BUILD_DIR)/$(TARGET).debug.elf
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $<

debug-qemu: $(BUILD_DIR)/$(TARGET).debug.elf
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $< \
		-S -s

gdb: $(BUILD_DIR)/$(TARGET).debug.elf
	@if [ -z "$(GDB)" ]; then \
		echo "No compatible debugger found."; \
		echo "Install one of: arm-none-eabi-gdb, gdb-multiarch, gdb"; \
		exit 1; \
	fi
	$(GDB) $< \
		-ex "target remote :1234" \
		-ex "hb kernel_main"

debug-help:
	@echo "Debug workflow:"
	@echo "  Debugger: $(if $(GDB),$(GDB),not found)"
	@echo "  1) make debug-build"
	@echo "  2) make debug-qemu   (terminal 1)"
	@echo "  3) make gdb          (terminal 2)"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run debug debug-build debug-run debug-qemu gdb debug-help clean size
