#include <stdint.h>
#include <stddef.h>

#include "uart.h"
#include "tracing_format.h"

/*
 * Minimal tracing backend for -finstrument-functions hooks.
 *
 * This is intentionally self-contained so it can be moved in/out of builds
 * without coupling to the rest of the application module layout.
 */

/*
 * Cppcheck is confused by the built-in __cyg_profile_func_enter/exit hooks and thinks they are declared incorrectly. Suppress the warnings about const parameters, which are actually non-const in the built-in hooks.
 * cppcheck-suppress constParameterPointer
 */

extern "C" void __attribute__((no_instrument_function))
// cppcheck-suppress constParameterPointer
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
// cppcheck-suppress constParameterPointer
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
