CROSS_COMPILE ?= arm-none-eabi-

CXX := $(CROSS_COMPILE)g++
CC  := $(CROSS_COMPILE)gcc
LD  := $(CROSS_COMPILE)g++
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size

TARGET := firmware

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
	-Wl,-Map=$(TARGET).map

LDLIBS := -lgcc

OBJS := startup.o main.o

all: $(TARGET).elf $(TARGET).bin size

startup.o: startup.S
	$(CC) $(ASFLAGS) -c $< -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

size: $(TARGET).elf
	$(SIZE) $<

run: $(TARGET).elf
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $(TARGET).elf

debug: $(TARGET).elf
	qemu-system-arm \
		-M virt \
		-cpu cortex-a15 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-kernel $(TARGET).elf \
		-S -s

clean:
	rm -f *.o $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all run debug clean size
