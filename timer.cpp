#include "timer.h"

#include "logic.h"

extern "C" uint32_t __attribute__((no_instrument_function)) arch_timer_freq_hz(void);
extern "C" uint64_t __attribute__((no_instrument_function)) arch_timer_count(void);

extern "C" void __attribute__((no_instrument_function)) delay_ms(uint32_t ms) {
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
