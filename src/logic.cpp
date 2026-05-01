#include "logic.h"

bool next_led_state(bool current) {
    return !current;
}

uint64_t ticks_from_ms(uint64_t freq_hz, uint32_t ms) {
    return (freq_hz * static_cast<uint64_t>(ms)) / 1000ULL;
}
