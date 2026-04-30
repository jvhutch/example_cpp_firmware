CROSS_COMPILE ?= arm-none-eabi-

CXX := $(CROSS_COMPILE)g++
CC  := $(CROSS_COMPILE)gcc
LD  := $(CROSS_COMPILE)g++
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size

TARGET := firmware
INSTRUMENT_FUNCTIONS ?= 0
INSTRUMENT_EXCLUDE_FILES ?=
INSTRUMENT_DEFAULT_EXCLUDE_FUNCTIONS ?= uart_putc,uart_write,mmio_read32,mmio_write32,delay_ms,arch_timer_count,arch_timer_freq_hz,__cyg_profile_func_enter,__cyg_profile_func_exit
INSTRUMENT_EXCLUDE_FUNCTIONS ?= $(INSTRUMENT_DEFAULT_EXCLUDE_FUNCTIONS)
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

ifeq ($(INSTRUMENT_FUNCTIONS),1)
CXXFLAGS += -finstrument-functions
ifneq ($(strip $(INSTRUMENT_EXCLUDE_FILES)),)
CXXFLAGS += -finstrument-functions-exclude-file-list=$(INSTRUMENT_EXCLUDE_FILES)
endif
ifneq ($(strip $(INSTRUMENT_EXCLUDE_FUNCTIONS)),)
CXXFLAGS += -finstrument-functions-exclude-function-list=$(INSTRUMENT_EXCLUDE_FUNCTIONS)
endif
endif

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

HOST_CXX ?= c++
HOST_CXXFLAGS ?= -std=c++17 -Wall -Wextra -O0 -g -I.
TEST_BUILD_DIR ?= $(BUILD_ROOT)/tests
TEST_BIN := $(TEST_BUILD_DIR)/unit_tests
TEST_TRACING_BIN := $(TEST_BUILD_DIR)/test_tracing

OBJS := $(BUILD_DIR)/startup.o $(BUILD_DIR)/main.o $(BUILD_DIR)/logic.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/timer_asm.o $(BUILD_DIR)/uart.o $(BUILD_DIR)/tracing_format.o $(BUILD_DIR)/tracing.o

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

$(BUILD_DIR)/logic.o: logic.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/timer.o: timer.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/timer_asm.o: timer_asm.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/uart.o: uart.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tracing.o: tracing.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/startup.debug.o: startup.S | $(BUILD_DIR)
	$(CC) $(DEBUG_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/main.debug.o: main.cpp | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/logic.debug.o: logic.cpp | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/timer.debug.o: timer.cpp | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/timer_asm.debug.o: timer_asm.S | $(BUILD_DIR)
	$(CC) $(DEBUG_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/uart.debug.o: uart.cpp | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tracing_format.o: tracing_format.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tracing.debug.o: tracing.cpp | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tracing_format.debug.o: tracing_format.cpp | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

DEBUG_OBJS := $(BUILD_DIR)/startup.debug.o $(BUILD_DIR)/main.debug.o $(BUILD_DIR)/logic.debug.o $(BUILD_DIR)/timer.debug.o $(BUILD_DIR)/timer_asm.debug.o $(BUILD_DIR)/uart.debug.o $(BUILD_DIR)/tracing_format.debug.o $(BUILD_DIR)/tracing.debug.o

$(TEST_BUILD_DIR):
	mkdir -p $@

$(TEST_BIN): tests/test_logic.cpp logic.cpp logic.h | $(TEST_BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) tests/test_logic.cpp logic.cpp -o $@

$(TEST_TRACING_BIN): tests/test_tracing.cpp tracing_format.cpp tracing_format.h | $(TEST_BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) tests/test_tracing.cpp tracing_format.cpp -o $@

test: $(TEST_BIN) $(TEST_TRACING_BIN)
	$(TEST_BIN)
	$(TEST_TRACING_BIN)

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
	@echo "Instrumentation examples:"
	@echo "  make INSTRUMENT_FUNCTIONS=1"
	@echo "  make INSTRUMENT_FUNCTIONS=1 INSTRUMENT_EXCLUDE_FUNCTIONS="
	@echo "  make INSTRUMENT_FUNCTIONS=1 INSTRUMENT_EXCLUDE_FILES=main.cpp"
	@echo "  make INSTRUMENT_FUNCTIONS=1 INSTRUMENT_EXCLUDE_FUNCTIONS=uart_putc,uart_write"

clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)

.PHONY: all run debug debug-build debug-run debug-qemu gdb debug-help clean size test
