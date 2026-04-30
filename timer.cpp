#include "timer.h"

static inline uint32_t __attribute__((no_instrument_function)) arch_timer_freq_hz(void) {
    uint32_t freq;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(freq));
    return freq;
}

static inline uint64_t __attribute__((no_instrument_function)) arch_timer_count(void) {
    uint32_t lo;
    uint32_t hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
}

extern "C" void __attribute__((no_instrument_function)) delay_ms(uint32_t ms) {
    if (ms == 0U) {
        return;
    }

    const uint64_t freq = static_cast<uint64_t>(arch_timer_freq_hz());
    const uint64_t ticks_to_wait = (freq * static_cast<uint64_t>(ms)) / 1000ULL;
    const uint64_t start = arch_timer_count();

    while ((arch_timer_count() - start) < ticks_to_wait) {
        asm volatile("nop");
    }
}
