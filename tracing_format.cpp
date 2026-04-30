#include <stdint.h>
#include <stddef.h>

#include "tracing_format.h"

size_t __attribute__((no_instrument_function))
trace_append_str(char *dst, size_t pos, size_t cap, const char *s) {
    while ((*s != '\0') && (pos < cap)) {
        dst[pos++] = *s++;
    }

    return pos;
}

size_t __attribute__((no_instrument_function))
trace_append_hex_uintptr(char *dst, size_t pos, size_t cap, uintptr_t value) {
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
