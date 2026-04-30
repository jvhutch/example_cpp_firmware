#pragma once

#include <stdint.h>

/*
 * Software watchdog built on the ARMv7 generic timer.
 *
 * This is a COOPERATIVE watchdog — expiry is only detected at the point
 * watchdog_pet() is called. It will not fire if the firmware enters a hard
 * lock (infinite loop, deadlock, fault handler spin) that never returns
 * control to the main loop. A hardware watchdog peripheral would be needed
 * to cover that case; the QEMU virt machine does not expose one.
 *
 * Normal operation:
 *   1. Call watchdog_init(timeout_ms) once at startup.
 *   2. Call watchdog_pet() regularly — faster than the timeout interval.
 *      If the deadline has already passed when watchdog_pet() is called,
 *      it invokes psci_system_reset() immediately.
 *
 * For immediate fault resets (e.g. __cxa_pure_virtual) call
 * psci_system_reset() directly rather than waiting for the watchdog.
 *
 * Reset mechanism:
 *   psci_system_reset() issues a PSCI SYSTEM_RESET (0x84000009) via HVC.
 *   The QEMU virt machine exposes PSCI 1.0 over HVC, so this causes an
 *   immediate full machine reset — equivalent to a power cycle.
 */

extern "C" void psci_system_reset(void);

void watchdog_init(uint32_t timeout_ms);
void watchdog_pet();
