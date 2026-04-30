/*
 * runtime.cpp
 *
 * Minimal C++ ABI runtime hooks for freestanding bare-metal builds.
 *
 * Because there is no hosted runtime, these handlers avoid unresolved
 * references if the compiler emits calls for pure virtual handling.
 */

#include "watchdog.h"

extern "C" void __cxa_pure_virtual(void) {
    /*
     * A pure virtual call is a programming error with no safe recovery path.
     * Reset immediately via PSCI rather than spinning, so the system comes
     * back up rather than hanging silently. This bypasses the cooperative
     * watchdog — we do not wait for the next watchdog_pet() call.
     *
     * The fallback spin is a safety net in case PSCI returns unexpectedly
     * (e.g. under a non-PSCI hypervisor or on hardware without PSCI support).
     */
    psci_system_reset();
    while (true) {
        /* fallback trap if PSCI returns unexpectedly */
    }
}
