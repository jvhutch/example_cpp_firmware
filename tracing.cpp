#include <stdint.h>
#include <stddef.h>

/*
 * Minimal tracing backend for -finstrument-functions hooks.
 *
 * This is intentionally self-contained so it can be moved in/out of builds
 * without coupling to the rest of the application module layout.
 */

extern "C" int uart_write(const char *data, size_t len);
extern "C" int uart_try_lock(void);
extern "C" void uart_unlock(void);
extern "C" int uart_write_unlocked(const char *data, size_t len);

static size_t __attribute__((no_instrument_function)) trace_append_str(char *dst, size_t pos, size_t cap, const char *s) {
    while ((*s != '\0') && (pos < cap)) {
        dst[pos++] = *s++;
    }

    return pos;
}

static size_t __attribute__((no_instrument_function)) trace_append_hex_uintptr(char *dst, size_t pos, size_t cap, uintptr_t value) {
    static const char hex_digits[] = "0123456789ABCDEF";

    if (pos < cap) {
        dst[pos++] = '0';
    }
    if (pos < cap) {
        dst[pos++] = 'x';
    }

    for (int shift = static_cast<int>(sizeof(uintptr_t) * 8U) - 4; shift >= 0; shift -= 4) {
        const uint32_t nibble = static_cast<uint32_t>((value >> static_cast<uint32_t>(shift)) & 0xFU);
        if (pos < cap) {
            dst[pos++] = hex_digits[nibble];
        }
    }

    return pos;
}

extern "C" void __attribute__((no_instrument_function))
__cyg_profile_func_enter(void *this_fn, void *call_site) {
    static volatile bool in_hook = false;
    char line[96];
    size_t pos = 0;

    if (in_hook) {
        return;
    }
    in_hook = true;

    pos = trace_append_str(line, pos, sizeof(line), "[I] -> fn=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), reinterpret_cast<uintptr_t>(this_fn));
    pos = trace_append_str(line, pos, sizeof(line), " from=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), reinterpret_cast<uintptr_t>(call_site));
    pos = trace_append_str(line, pos, sizeof(line), "\n");
    if (uart_try_lock() != 0) {
        (void)uart_write_unlocked(line, pos);
        uart_unlock();
    }

    in_hook = false;
}

extern "C" void __attribute__((no_instrument_function))
__cyg_profile_func_exit(void *this_fn, void *call_site) {
    static volatile bool in_hook = false;
    char line[96];
    size_t pos = 0;

    if (in_hook) {
        return;
    }
    in_hook = true;

    pos = trace_append_str(line, pos, sizeof(line), "[I] <- fn=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), reinterpret_cast<uintptr_t>(this_fn));
    pos = trace_append_str(line, pos, sizeof(line), " from=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), reinterpret_cast<uintptr_t>(call_site));
    pos = trace_append_str(line, pos, sizeof(line), "\n");
    if (uart_try_lock() != 0) {
        (void)uart_write_unlocked(line, pos);
        uart_unlock();
    }

    in_hook = false;
}
