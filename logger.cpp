#include "logger.h"
#include "uart.h"

void Logger::write(const char *msg) {
    if (msg == nullptr) {
        return;
    }
    uart_write(msg, str_len(msg));
}

void Logger::write_line(const char *msg) {
    if (msg == nullptr) {
        return;
    }
    uart_write_line(msg, str_len(msg));
}

void Logger::write_hex_u32(const char *label, uint32_t value) {
    char line[64];
    size_t pos = 0;

    pos = append_str(line, pos, sizeof(line), label);
    pos = append_str(line, pos, sizeof(line), "=0x");

    for (int shift = 28; shift >= 0; shift -= 4) {
        const char nibble = static_cast<char>((value >> shift) & 0xFU);
        if ((pos + 1U) < sizeof(line)) {
            line[pos++] = (nibble < 10) ? static_cast<char>('0' + nibble)
                                        : static_cast<char>('a' + nibble - 10);
        }
    }

    if (pos < sizeof(line)) {
        line[pos] = '\0';
    } else {
        line[sizeof(line) - 1U] = '\0';
    }

    write_line(line);
}

void Logger::write_key_u32_ms(const char *key, uint32_t value) {
    char line[96];
    size_t pos = 0;

    pos = append_str(line, pos, sizeof(line), key);
    pos = append_str(line, pos, sizeof(line), "=");
    pos = append_u32_dec(line, pos, sizeof(line), value);
    pos = append_str(line, pos, sizeof(line), " ms");

    if (pos < sizeof(line)) {
        line[pos] = '\0';
    } else {
        line[sizeof(line) - 1U] = '\0';
    }

    write_line(line);
}

size_t Logger::append_str(char *dst, size_t pos, size_t cap, const char *s) {
    while ((*s != '\0') && ((pos + 1U) < cap)) {
        dst[pos++] = *s++;
    }
    return pos;
}

size_t Logger::append_u32_dec(char *dst, size_t pos, size_t cap, uint32_t value) {
    char tmp[10];
    size_t n = 0;

    do {
        tmp[n++] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
    } while ((value != 0U) && (n < sizeof(tmp)));

    while ((n > 0U) && ((pos + 1U) < cap)) {
        dst[pos++] = tmp[--n];
    }

    return pos;
}

size_t Logger::str_len(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}
