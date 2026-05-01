#pragma once

#include <stdint.h>

bool next_led_state(bool current);
uint64_t ticks_from_ms(uint64_t freq_hz, uint32_t ms);
