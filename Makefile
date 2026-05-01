CROSS_COMPILE ?= arm-none-eabi-

CXX := $(CROSS_COMPILE)g++
CC  := $(CROSS_COMPILE)gcc
LD  := $(CROSS_COMPILE)g++
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size

TARGET := firmware
SRC_DIR := src
INC_DIR := include
ARCH_DIR := $(SRC_DIR)/arch/arm
INSTRUMENT_FUNCTIONS ?= 0
INSTRUMENT_EXCLUDE_FILES ?= boot_config.cpp,uart.cpp,watchdog.cpp,timer.cpp,tracing.cpp,tracing_format.cpp
INSTRUMENT_EXCLUDE_FUNCTIONS ?=

# Detect operating system
UNAME_S := $(shell uname -s)

# Default build directory
BUILD_ROOT		?= $(PWD)/build
BUILD_DIR		?= $(BUILD_ROOT)/artifacts
RAM_DISK_SIZE		?= 4M

TARGET_ELF := $(BUILD_DIR)/$(TARGET).elf
TARGET_BIN := $(BUILD_DIR)/$(TARGET).bin
TARGET_DTB := $(BUILD_DIR)/virt.dtb
TARGET_RUNTIME_DTB := $(BUILD_DIR)/virt.runtime.dtb
DTB_LOAD_ADDR ?= 0x44000000
USE_QEMU_DTB ?= 0
BOOTARGS ?=
QEMU_EXTRA_ARGS ?=
WATCHDOG_TEST_BOOTARGS ?= watchdog_timeout_ms=2001 loop_delay_ms=2000 force_watchdog_timeout_once=1

ifeq ($(USE_QEMU_DTB),1)
QEMU_DTB_ARGS := -dtb $(TARGET_RUNTIME_DTB) -device loader,file=$(TARGET_RUNTIME_DTB),addr=$(DTB_LOAD_ADDR),force-raw=on
else
QEMU_DTB_ARGS := -device loader,file=$(TARGET_RUNTIME_DTB),addr=$(DTB_LOAD_ADDR),force-raw=on
endif

CPUFLAGS := -mcpu=cortex-a15 -marm

CXXFLAGS := \
	$(CPUFLAGS) \
	-std=c++17 \
	-I$(INC_DIR) \
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
HOST_CXXFLAGS ?= -std=c++17 -Wall -Wextra -O0 -g -I. -I$(INC_DIR)
TEST_BUILD_DIR ?= $(BUILD_ROOT)/tests
TEST_BIN := $(TEST_BUILD_DIR)/unit_tests
TEST_TRACING_BIN := $(TEST_BUILD_DIR)/test_tracing
TEST_BOOT_CONFIG_BIN := $(TEST_BUILD_DIR)/test_boot_config
TEST_WATCHDOG_BIN := $(TEST_BUILD_DIR)/test_watchdog

OBJ_SRC  := $(BUILD_DIR)/src
OBJ_ARCH := $(BUILD_DIR)/arch/arm

OBJS := \
	$(OBJ_ARCH)/startup.o \
	$(OBJ_SRC)/main.o \
	$(OBJ_SRC)/logic.o \
	$(OBJ_SRC)/timer.o \
	$(OBJ_ARCH)/timer.o \
	$(OBJ_SRC)/uart.o \
	$(OBJ_SRC)/tracing_format.o \
	$(OBJ_SRC)/tracing.o \
	$(OBJ_SRC)/watchdog.o \
	$(OBJ_SRC)/boot_config.o \
	$(OBJ_SRC)/logger.o \
	$(OBJ_ARCH)/reset.o \
	$(OBJ_SRC)/runtime.o

all: $(TARGET_ELF) $(TARGET_BIN) size

$(BUILD_DIR): $(MAKEFILE_LIST)
	mkdir -p $(BUILD_ROOT)
ifeq ($(UNAME_S),Darwin)
	mount -t tmpfs | grep -q $(BUILD_ROOT) || sudo mount_tmpfs -s $(RAM_DISK_SIZE) $(BUILD_ROOT)
	touch $(BUILD_ROOT)/.metadata_never_index
endif
	mkdir -p $(BUILD_DIR)

$(OBJ_SRC)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_SRC)/%.debug.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

$(OBJ_ARCH)/%.o: $(ARCH_DIR)/%.S | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(OBJ_ARCH)/%.debug.o: $(ARCH_DIR)/%.S | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(DEBUG_ASFLAGS) -c $< -o $@

DEBUG_OBJS := \
	$(OBJ_ARCH)/startup.debug.o \
	$(OBJ_SRC)/main.debug.o \
	$(OBJ_SRC)/logic.debug.o \
	$(OBJ_SRC)/timer.debug.o \
	$(OBJ_ARCH)/timer.debug.o \
	$(OBJ_SRC)/uart.debug.o \
	$(OBJ_SRC)/tracing_format.debug.o \
	$(OBJ_SRC)/tracing.debug.o \
	$(OBJ_SRC)/watchdog.debug.o \
	$(OBJ_SRC)/boot_config.debug.o \
	$(OBJ_SRC)/logger.debug.o \
	$(OBJ_ARCH)/reset.debug.o \
	$(OBJ_SRC)/runtime.debug.o

$(TEST_BUILD_DIR):
	mkdir -p $@

$(TEST_BIN): tests/test_logic.cpp $(SRC_DIR)/logic.cpp $(INC_DIR)/logic.h | $(TEST_BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) tests/test_logic.cpp $(SRC_DIR)/logic.cpp -o $@

$(TEST_TRACING_BIN): tests/test_tracing.cpp $(SRC_DIR)/tracing_format.cpp $(INC_DIR)/tracing_format.h | $(TEST_BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) tests/test_tracing.cpp $(SRC_DIR)/tracing_format.cpp -o $@

$(TEST_BOOT_CONFIG_BIN): tests/test_boot_config.cpp $(SRC_DIR)/boot_config.cpp $(INC_DIR)/boot_config.h | $(TEST_BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) tests/test_boot_config.cpp $(SRC_DIR)/boot_config.cpp -o $@

$(TEST_WATCHDOG_BIN): tests/test_watchdog.cpp $(SRC_DIR)/watchdog.cpp $(INC_DIR)/watchdog.h $(SRC_DIR)/logic.cpp $(INC_DIR)/logic.h $(INC_DIR)/timer.h $(INC_DIR)/logger.h | $(TEST_BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) tests/test_watchdog.cpp $(SRC_DIR)/watchdog.cpp $(SRC_DIR)/logic.cpp -o $@

test: $(TEST_BIN) $(TEST_TRACING_BIN) $(TEST_BOOT_CONFIG_BIN) $(TEST_WATCHDOG_BIN)
	$(TEST_BIN)
	$(TEST_TRACING_BIN)
	$(TEST_BOOT_CONFIG_BIN)
	$(TEST_WATCHDOG_BIN)

$(BUILD_DIR)/$(TARGET).debug.elf: $(DEBUG_OBJS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(DEBUG_OBJS) $(LDLIBS) -o $@

$(BUILD_DIR)/$(TARGET).debug.bin: $(BUILD_DIR)/$(TARGET).debug.elf
	$(OBJCOPY) -O binary $< $@

$(TARGET_ELF): $(OBJS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

$(TARGET_DTB): | $(BUILD_DIR)
	qemu-system-arm -M virt,dumpdtb=$@ -display none -nodefaults

$(TARGET_RUNTIME_DTB): $(TARGET_DTB) | $(BUILD_DIR)
	cp $(TARGET_DTB) $@
ifneq ($(strip $(BOOTARGS)),)
	fdtput -t s $@ /chosen bootargs "$(BOOTARGS)"
endif

size: $(TARGET_ELF)
	$(SIZE) $<

run: $(TARGET_ELF) $(TARGET_RUNTIME_DTB)
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $(TARGET_ELF) \
		$(QEMU_DTB_ARGS) \
		$(QEMU_EXTRA_ARGS)

run-no-reboot: QEMU_EXTRA_ARGS += -no-reboot
run-no-reboot: run

watchdog-timeout-test:
	$(MAKE) run-no-reboot INSTRUMENT_FUNCTIONS=0 BOOTARGS='$(WATCHDOG_TEST_BOOTARGS)'

debug: $(TARGET_ELF) $(TARGET_RUNTIME_DTB)
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $(TARGET_ELF) \
		$(QEMU_DTB_ARGS) \
		-S -s

debug-build: $(BUILD_DIR)/$(TARGET).debug.elf $(BUILD_DIR)/$(TARGET).debug.bin

debug-run: $(BUILD_DIR)/$(TARGET).debug.elf $(TARGET_RUNTIME_DTB)
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $< \
		$(QEMU_DTB_ARGS)

debug-qemu: $(BUILD_DIR)/$(TARGET).debug.elf $(TARGET_RUNTIME_DTB)
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $< \
		$(QEMU_DTB_ARGS) \
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
	@echo "DTB/bootargs examples:"
	@echo "  BOOTARGS are embedded into a runtime DTB under /chosen/bootargs"
	@echo "  make run"
	@echo "  make run USE_QEMU_DTB=1 BOOTARGS='watchdog_timeout_ms=100 loop_delay_ms=1000'"
	@echo "  make run-no-reboot"
	@echo "Watchdog timeout test (deterministic):"
	@echo "  make watchdog-timeout-test"
	@echo "  make watchdog-timeout-test WATCHDOG_TEST_BOOTARGS='watchdog_timeout_ms=120 loop_delay_ms=100 force_watchdog_timeout_once=1'"
	@echo "Instrumentation examples:"
	@echo "  make INSTRUMENT_FUNCTIONS=1"
	@echo "  make INSTRUMENT_FUNCTIONS=1 INSTRUMENT_EXCLUDE_FUNCTIONS="
	@echo "  make INSTRUMENT_FUNCTIONS=1 INSTRUMENT_EXCLUDE_FILES=main.cpp"
	@echo "  make INSTRUMENT_FUNCTIONS=1 INSTRUMENT_EXCLUDE_FUNCTIONS=uart_putc,uart_write"

clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)

distclean: clean ## Remove outputs and unmount temp filesystem on macOS
ifeq ($(UNAME_S),Darwin)
	mount -t tmpfs | grep -q $(BUILD_ROOT) && diskutil umount force $(BUILD_ROOT) || true
else
	rmdir -p $(BUILD_DIR)
endif

.PHONY: all run run-no-reboot watchdog-timeout-test debug debug-build debug-run debug-qemu gdb debug-help clean distclean size test
