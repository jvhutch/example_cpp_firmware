/*
 * timer_asm.S
 *
 * ARMv7-A generic timer accessors implemented in assembly.
 */

.syntax unified
.cpu cortex-a15
.arm

.global arch_timer_freq_hz
.type arch_timer_freq_hz, %function
arch_timer_freq_hz:
    mrc p15, 0, r0, c14, c0, 0
    bx lr
.size arch_timer_freq_hz, .-arch_timer_freq_hz

.global arch_timer_count
.type arch_timer_count, %function
arch_timer_count:
    mrrc p15, 0, r0, r1, c14
    bx lr
.size arch_timer_count, .-arch_timer_count
