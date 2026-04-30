/*
 * main.cpp
 *
 * Bare-metal C++ firmware-style program for QEMU ARM virt machine.
 *
 * Target:
 *   qemu-system-arm -M virt -cpu cortex-a15
 *
 * Important:
 *   This does not run under Linux.
 *   There are no syscalls, no libc startup, no stdout, and no OS timer.
 *
 * Output is sent directly to QEMU's emulated PL011 UART.
 */

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------
 * QEMU ARM virt machine hardware addresses
 * ------------------------------------------------------------
 *
 * QEMU's ARM virt machine exposes a PL011-compatible UART.
 *
 * For the 32-bit ARM virt machine, UART0 is commonly located at:
 *
 *   0x09000000
 *
 * This address is QEMU-machine-specific. On real hardware, this would
 * come from the SoC reference manual, device tree, or board support package.
 */

#define UART0_BASE      0x09000000UL
#define UART_DR_OFFSET  0x000U
#define UART_FR_OFFSET  0x018U

#define UART_FR_TXFF    (1U << 5)   /* Transmit FIFO full */

/* ------------------------------------------------------------
 * Minimal MMIO helpers
 * ------------------------------------------------------------ */

static inline void mmio_write32(uintptr_t addr, uint32_t value) {
    /*
     * volatile is essential for memory-mapped I/O.
     *
     * It tells the compiler that this store has externally visible side
     * effects and must not be optimized away.
     */
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

static volatile uint32_t uart_write_lock = 0U;

static inline void __attribute__((no_instrument_function)) uart_lock_acquire(void) {
    while (__sync_lock_test_and_set(&uart_write_lock, 1U) != 0U) {
        /* spin */
    }
}

static inline void __attribute__((no_instrument_function)) uart_lock_release(void) {
    __sync_lock_release(&uart_write_lock);
}

extern "C" void __attribute__((no_instrument_function)) uart_putc(char c);

extern "C" int __attribute__((no_instrument_function)) uart_try_lock(void) {
    return (__sync_lock_test_and_set(&uart_write_lock, 1U) == 0U) ? 1 : 0;
}

extern "C" void __attribute__((no_instrument_function)) uart_unlock(void) {
    uart_lock_release();
}

extern "C" int __attribute__((no_instrument_function)) uart_write_unlocked(const char *data, size_t len) {
    if (data == nullptr) {
        return -1;
    }

    for (size_t i = 0; i < len; ++i) {
        if (data[i] == '\n') {
            uart_putc('\r');
        }

        uart_putc(data[i]);
    }

    return static_cast<int>(len);
}

/* ------------------------------------------------------------
 * C-compatible hardware abstraction layer
 * ------------------------------------------------------------ */

extern "C" void __attribute__((no_instrument_function)) uart_putc(char c) {
    /*
     * Wait while the UART transmit FIFO is full.
     *
     * Some simple QEMU examples skip this because QEMU is forgiving, but
     * checking the flag is closer to real firmware behavior.
     */
    while ((mmio_read32(UART0_BASE + UART_FR_OFFSET) & UART_FR_TXFF) != 0U) {
        /* spin */
    }

    mmio_write32(UART0_BASE + UART_DR_OFFSET, static_cast<uint32_t>(c));
}

extern "C" int __attribute__((no_instrument_function)) uart_write(const char *data, size_t len) {
    if (data == nullptr) {
        return -1;
    }

    uart_lock_acquire();

    const int written = uart_write_unlocked(data, len);

    uart_lock_release();

    return written;
}

extern "C" int __attribute__((no_instrument_function)) uart_write_line(const char *data, size_t len) {
    if (data == nullptr) {
        return -1;
    }

    uart_lock_acquire();

    for (size_t i = 0; i < len; ++i) {
        if (data[i] == '\n') {
            uart_putc('\r');
        }

        uart_putc(data[i]);
    }

    uart_putc('\r');
    uart_putc('\n');

    uart_lock_release();

    return static_cast<int>(len);
}

extern "C" void gpio_set_led(bool on) {
    /*
     * QEMU virt does not give us a simple board LED like many MCU boards do.
     * So this simulates LED state over UART.
     */
    if (on) {
        const char msg[] = "[GPIO] simulated LED = 1\n";
        uart_write(msg, sizeof(msg) - 1U);
    } else {
        const char msg[] = "[GPIO] simulated LED = 0\n";
        uart_write(msg, sizeof(msg) - 1U);
    }
}

/*
 * ARMv7-A generic timer helpers.
 *
 * QEMU virt with Cortex-A15 exposes the architected generic timer via
 * system registers. This gives us hardware-timed delays in real units
 * instead of host-speed-dependent busy-loop cycle guesses.
 */
static inline uint32_t arch_timer_freq_hz(void) {
    uint32_t freq;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(freq));
    return freq;
}

static inline uint64_t arch_timer_count(void) {
    uint32_t lo;
    uint32_t hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
}

extern "C" void delay_ms(uint32_t ms) {
    if (ms == 0U) {
        return;
    }

    const uint64_t freq = static_cast<uint64_t>(arch_timer_freq_hz());
    const uint64_t ticks_to_wait = (freq * static_cast<uint64_t>(ms)) / 1000ULL;
    const uint64_t start = arch_timer_count();

    while ((arch_timer_count() - start) < ticks_to_wait) {
        asm volatile("nop");
    }
}

/* ------------------------------------------------------------
 * Small fixed-size logger
 * ------------------------------------------------------------ */

class Logger {
public:
    void write(const char *msg) {
        if (msg == nullptr) {
            return;
        }

        size_t len = 0;
        while (msg[len] != '\0') {
            ++len;
        }

        uart_write(msg, len);
    }

    void write_line(const char *msg) {
        if (msg == nullptr) {
            return;
        }

        size_t len = 0;
        while (msg[len] != '\0') {
            ++len;
        }

        uart_write_line(msg, len);
    }
};

/* ------------------------------------------------------------
 * LED blinker state machine
 * ------------------------------------------------------------ */

class LedBlinker {
public:
    explicit LedBlinker(Logger &logger)
        : logger_(logger),
          led_on_(false) {}

    void toggle() {
        led_on_ = !led_on_;
        gpio_set_led(led_on_);

        if (led_on_) {
            logger_.write_line("LED ON");
        } else {
            logger_.write_line("LED OFF");
        }
    }

private:
    Logger &logger_;
    bool led_on_;
};

/* ------------------------------------------------------------
 * Application object
 * ------------------------------------------------------------ */

class Application {
public:
    Application()
        : logger_(),
          blinker_(logger_) {}

    void init() {
        logger_.write_line("");
        logger_.write_line("Bare-metal C++ application started on QEMU");
        logger_.write_line("Output is using the emulated PL011 UART");
        gpio_set_led(false);
    }

    void run_forever() {
        while (true) {
            blinker_.toggle();

            delay_ms(500U);
        }
    }

private:
    Logger logger_;
    LedBlinker blinker_;
};

/* ------------------------------------------------------------
 * Static application instance
 * ------------------------------------------------------------ */

static Application app;

/* ------------------------------------------------------------
 * Called from startup.S after low-level CPU setup.
 * ------------------------------------------------------------ */

extern "C" void kernel_main(void) {
    app.init();
    app.run_forever();
}

/* ------------------------------------------------------------
 * Minimal C++ runtime hooks
 * ------------------------------------------------------------
 *
 * Because this is freestanding bare-metal C++, there is no normal hosted
 * runtime. These handlers avoid unresolved references if the compiler emits
 * calls for pure virtual handling or stack protector failure.
 */

extern "C" void __cxa_pure_virtual(void) {
    while (true) {
        /* trap */
    }
}

extern "C" void __stack_chk_fail(void) {
    while (true) {
        /* trap */
    }
}
