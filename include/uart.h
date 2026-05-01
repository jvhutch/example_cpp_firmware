#pragma once

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#define NO_INSTRUMENT __attribute__((no_instrument_function))
#else
#define NO_INSTRUMENT
#endif

#ifdef __cplusplus
extern "C" {
#endif

NO_INSTRUMENT void uart_putc(char c);
NO_INSTRUMENT int uart_try_lock(void);
NO_INSTRUMENT void uart_unlock(void);
NO_INSTRUMENT int uart_write_unlocked(const char *data, size_t len);
NO_INSTRUMENT int uart_write(const char *data, size_t len);
NO_INSTRUMENT int uart_write_line(const char *data, size_t len);
void gpio_set_led(bool on);

#ifdef __cplusplus
}
#endif
