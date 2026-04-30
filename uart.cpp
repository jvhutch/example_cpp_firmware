#include "uart.h"

#include <stdint.h>

/* ------------------------------------------------------------
 * QEMU ARM virt machine hardware addresses
 * ------------------------------------------------------------ */

#define UART0_BASE      0x09000000UL
#define UART_DR_OFFSET  0x000U
#define UART_FR_OFFSET  0x018U

#define UART_FR_TXFF    (1U << 5)   /* Transmit FIFO full */

/* ------------------------------------------------------------
 * Minimal MMIO helpers
 * ------------------------------------------------------------ */

static inline void mmio_write32(uintptr_t addr, uint32_t value) {
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
    if (on) {
        const char msg[] = "[GPIO] simulated LED = 1\n";
        uart_write(msg, sizeof(msg) - 1U);
    } else {
        const char msg[] = "[GPIO] simulated LED = 0\n";
        uart_write(msg, sizeof(msg) - 1U);
    }
}
