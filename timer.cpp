#include "timer.h"

#include "logic.h"



void delay_ms(uint32_t ms) {
    if (ms == 0U) {
        return;
    }

    const uint64_t freq = static_cast<uint64_t>(arch_timer_freq_hz());
    const uint64_t ticks_to_wait = ticks_from_ms(freq, ms);
    const uint64_t start = arch_timer_count();

    while ((arch_timer_count() - start) < ticks_to_wait) {
        /* spin */
    }
}
