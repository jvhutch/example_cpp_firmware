#pragma once

#include <stddef.h>

extern "C" void __attribute__((no_instrument_function)) uart_putc(char c);
extern "C" int __attribute__((no_instrument_function)) uart_try_lock(void);
extern "C" void __attribute__((no_instrument_function)) uart_unlock(void);
extern "C" int __attribute__((no_instrument_function)) uart_write_unlocked(const char *data, size_t len);
extern "C" int __attribute__((no_instrument_function)) uart_write(const char *data, size_t len);
extern "C" int __attribute__((no_instrument_function)) uart_write_line(const char *data, size_t len);
extern "C" void gpio_set_led(bool on);
