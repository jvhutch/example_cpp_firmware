#pragma once

#include <stdint.h>
#include <stddef.h>

class Logger {
public:
    static void write(const char *msg);
    static void write_line(const char *msg);
    static void write_hex_u32(const char *label, uint32_t value);
    static void write_key_u32_ms(const char *key, uint32_t value);

private:
    static size_t append_str(char *dst, size_t pos, size_t cap, const char *s);
    static size_t append_u32_dec(char *dst, size_t pos, size_t cap, uint32_t value);
    static size_t str_len(const char *s);
};
