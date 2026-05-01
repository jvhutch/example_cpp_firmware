#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t arch_timer_freq_hz(void);
uint64_t arch_timer_count(void);
void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif
