#include <stdint.h>

#include "watchdog.h"
#include "logic.h"
#include "logger.h"

extern "C" uint32_t arch_timer_freq_hz(void);
extern "C" uint64_t arch_timer_count(void);

static uint64_t s_timeout_ticks = 0U;
static uint64_t s_deadline      = 0U;

void watchdog_init(uint32_t timeout_ms) {
    const uint64_t freq = static_cast<uint64_t>(arch_timer_freq_hz());
    s_timeout_ticks = ticks_from_ms(freq, timeout_ms);
    s_deadline      = arch_timer_count() + s_timeout_ticks;
}

void watchdog_pet() {
    const uint64_t now = arch_timer_count();

    /*
     * Check expiry first. If the deadline has passed — because the main loop
     * was late, or this is the first pet after a long stall — treat it as a
     * timeout and reset immediately. This is a cooperative check: a hard lock
     * that never reaches this function will not be caught here.
     */
    if (now >= s_deadline) {
        Logger::write_line("watchdog_timeout_warning=deadline_expired_resetting");
        psci_system_reset();
    }

    s_deadline = now + s_timeout_ticks;
}
