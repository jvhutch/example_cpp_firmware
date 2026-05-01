# QEMU ARM Bare-Metal C++ Example

A freestanding C++ firmware-style project for the QEMU ARM virt machine.

This project boots without an OS, initializes memory in startup assembly, runs C++ global constructors, and executes an application loop that toggles a simulated GPIO LED and writes output through the emulated PL011 UART.

## Highlights

- Bare-metal ARMv7-A target using QEMU virt with Cortex-A15
- No libc startup, syscalls, or hosted runtime assumptions
- C++ application logic with manual low-level startup and linker script
- UART output with lock-protected line writes
- Optional function instrumentation using GCC -finstrument-functions
- Tracing module separated from application logic
- Timer register access moved into assembly module to avoid C++ inline-assembly analyzer issues

## Repository Layout

- `src/main.cpp`:
  - Application classes and control flow
  - `kernel_main` entry point invoked by startup code
  - Minimal runtime trap hooks (`__cxa_pure_virtual`, `__stack_chk_fail`)
- `src/arch/arm/startup.S`:
  - Reset/start entry (`_start`)
  - Stack setup
  - `.bss` zeroing
  - C++ constructor iteration over `.init_array`
  - Branch to `kernel_main`
- `linker.ld`:
  - Memory layout for QEMU virt RAM
  - Section placement (`.text`, `.rodata`, `.init_array`, `.data`, `.bss`)
  - Symbols used by startup code
- `include/uart.h`, `src/uart.cpp`:
  - PL011 MMIO UART backend
  - Locking primitives for serialized output
  - Locked and unlocked write paths
  - Simulated GPIO LED output over UART
- `include/timer.h`, `src/timer.cpp`, `src/arch/arm/timer_asm.S`:
  - Delay API (`delay_ms`) in C++
  - ARM generic timer register reads implemented in assembly
- `include/logic.h`, `src/logic.cpp`:
  - Pure logic helpers shared by firmware and host unit tests
  - LED state transitions and timer conversion math
- `src/tracing.cpp`:
  - `__cyg_profile_func_enter` and `__cyg_profile_func_exit`
  - Non-blocking try-lock trace emission to avoid lockup/deadlock
- `tests/test_logic.cpp`:
  - Native host unit tests for pure logic helpers
- `Makefile`:
  - Cross toolchain build rules
  - Run and debug targets
  - Instrumentation controls and exclusions

## Requirements

- Host tools:
  - `make`
  - `qemu-system-arm`
- ARM bare-metal toolchain:
  - `arm-none-eabi-gcc`
  - `arm-none-eabi-g++`
  - `arm-none-eabi-objcopy`
  - `arm-none-eabi-size`
  - Optional: `arm-none-eabi-gdb` (or `gdb-multiarch`)

The Makefile defaults to the `arm-none-eabi-` prefix:

```make
CROSS_COMPILE ?= arm-none-eabi-
```

You can override it when invoking make if needed.

## Build and Run

Build release firmware:

```bash
make
```

Run on QEMU:

```bash
make run
```

Clean artifacts:

```bash
make clean
```

## Instrumentation

Enable function instrumentation:

```bash
make clean && make INSTRUMENT_FUNCTIONS=1
make run INSTRUMENT_FUNCTIONS=1
```

Key points:

- Instrumentation hooks are implemented in `src/tracing.cpp`.
- Trace output uses `uart_try_lock` and drops a trace line if the lock is not immediately available.
- This design avoids blocking from hook context and prevents startup/reentrancy lockups.

Controls in Makefile:

- `INSTRUMENT_FUNCTIONS=1` to enable hooks
- `INSTRUMENT_EXCLUDE_FILES` to exclude by file
- `INSTRUMENT_EXCLUDE_FUNCTIONS` to exclude by function list

Default function exclusions include low-level I/O and trace hooks.

## Debug Workflow

Build debug image:

```bash
make debug-build
```

Start QEMU waiting for debugger:

```bash
make debug-qemu
```

Attach debugger (separate terminal):

```bash
make gdb
```

Print helper text:

```bash
make debug-help
```

## Unit Testing

This project includes a native host-side unit test target for pure logic code.

Run tests:

```bash
make test
```

Notes:

- Tests are built with host `c++`, not `arm-none-eabi-g++`.
- The test binary is written to `build/tests/unit_tests`.
- Firmware-facing code that uses MMIO or startup assembly is validated by QEMU integration runs, while pure logic is validated by `make test`.

## Runtime Behavior

At boot, the firmware:

1. Sets up stack and clears `.bss`
2. Runs C++ global constructors
3. Calls `kernel_main`
4. Prints startup lines
5. Toggles simulated LED state every 500 ms and prints status

Typical output includes lines such as:

- `Bare-metal C++ application started on QEMU`
- `Output is using the emulated PL011 UART`
- `[GPIO] simulated LED = 1` / `0`
- `LED ON` / `LED OFF`
- Optional instrumentation lines prefixed with `[I]`

## Architecture Notes

- This is a freestanding build (`-ffreestanding`, `-nostdlib`).
- Linker script places image at `0x40000000` in QEMU virt RAM.
- UART base address is PL011-compatible for this machine configuration.
- The linker may emit a warning about RWX load segment in this minimal layout; this is expected for the current script.

## Common Commands

```bash
# Release build
make

# Release run
make run

# Instrumented build and run
make clean && make INSTRUMENT_FUNCTIONS=1
make run INSTRUMENT_FUNCTIONS=1

# Debug build and debug session
make debug-build
make debug-qemu
make gdb

# Clean
make clean
```

## Extending the Project

Suggested next additions:

- Add unit-testable logic modules independent from hardware
- Introduce board abstraction interfaces for UART/GPIO/timer
- Add symbolized trace decoding helpers
- Harden linker script memory permissions and section granularity
