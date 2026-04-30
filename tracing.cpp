#include <stdint.h>
#include <stddef.h>

/*
 * Minimal tracing backend for -finstrument-functions hooks.
 *
 * This is intentionally self-contained so it can be moved in/out of builds
 * without coupling to the rest of the application module layout.
 */

#define UART0_BASE      0x09000000UL
#define UART_DR_OFFSET  0x000U
#define UART_FR_OFFSET  0x018U
#define UART_FR_TXFF    (1U << 5)

static void __attribute__((no_instrument_function)) trace_putc_raw(char c) {
    while ((*(volatile uint32_t *)(UART0_BASE + UART_FR_OFFSET) & UART_FR_TXFF) != 0U) {
        /* spin */
    }

    *(volatile uint32_t *)(UART0_BASE + UART_DR_OFFSET) = static_cast<uint32_t>(c);
}

static void __attribute__((no_instrument_function)) trace_puts_raw(const char *s) {
    while (*s != '\0') {
        trace_putc_raw(*s++);
    }
}

static void __attribute__((no_instrument_function)) trace_put_hex_uintptr(uintptr_t value) {
    static const char hex_digits[] = "0123456789ABCDEF";

    trace_puts_raw("0x");
    for (int shift = static_cast<int>(sizeof(uintptr_t) * 8U) - 4; shift >= 0; shift -= 4) {
        const uint32_t nibble = static_cast<uint32_t>((value >> static_cast<uint32_t>(shift)) & 0xFU);
        trace_putc_raw(hex_digits[nibble]);
    }
}

extern "C" void __attribute__((no_instrument_function))
__cyg_profile_func_enter(void *this_fn, void *call_site) {
    static volatile bool in_hook = false;

    if (in_hook) {
        return;
    }
    in_hook = true;

    trace_puts_raw("[I] -> fn=");
    trace_put_hex_uintptr(reinterpret_cast<uintptr_t>(this_fn));
    trace_puts_raw(" from=");
    trace_put_hex_uintptr(reinterpret_cast<uintptr_t>(call_site));
    trace_puts_raw("\r\n");

    in_hook = false;
}

extern "C" void __attribute__((no_instrument_function))
__cyg_profile_func_exit(void *this_fn, void *call_site) {
    static volatile bool in_hook = false;

    if (in_hook) {
        return;
    }
    in_hook = true;

    trace_puts_raw("[I] <- fn=");
    trace_put_hex_uintptr(reinterpret_cast<uintptr_t>(this_fn));
    trace_puts_raw(" from=");
    trace_put_hex_uintptr(reinterpret_cast<uintptr_t>(call_site));
    trace_puts_raw("\r\n");

    in_hook = false;
}
