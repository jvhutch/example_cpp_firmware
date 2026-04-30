/*
 * reset_asm.S
 *
 * System reset via PSCI SYSTEM_RESET (function ID 0x84000009).
 *
 * The QEMU virt machine exposes PSCI 1.0 over HVC. On real Cortex-A hardware
 * the call convention is identical; only the conduit (hvc vs smc) may differ.
 */

.syntax unified
.cpu cortex-a15
.arm

.global psci_system_reset
.type psci_system_reset, %function
psci_system_reset:
    ldr r0, =0x84000009     /* PSCI SYSTEM_RESET */
    hvc #0
    /* Should not return; spin as a safety net */
0:
    b 0b
.size psci_system_reset, .-psci_system_reset
