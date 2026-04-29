/*
 * startup.S
 *
 * Minimal ARMv7-A startup code for QEMU ARM virt.
 *
 * Responsibilities:
 *   - Provide an entry symbol
 *   - Set up a stack pointer
 *   - Zero the .bss section
 *   - Call C++ global constructors
 *   - Call kernel_main()
 */

.syntax unified
.cpu cortex-a15
.arm

.global _start
.extern kernel_main
.extern __bss_start__
.extern __bss_end__
.extern __init_array_start
.extern __init_array_end

_start:
    /*
     * Set up a stack.
     *
     * The linker script defines __stack_top__ near the top of RAM.
     */
    ldr sp, =__stack_top__

    /*
     * Zero .bss.
     *
     * This is normally done by the C runtime before main().
     * In bare-metal firmware, startup code must do it explicitly.
     */
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0

zero_bss:
    cmp r0, r1
    bhs call_constructors
    str r2, [r0], #4
    b zero_bss

call_constructors:
    /*
     * Call global/static C++ constructors.
     *
     * This is needed because main.cpp contains:
     *
     *   static Application app;
     *
     * Without this loop, app's constructor would not run.
     */
    ldr r0, =__init_array_start
    ldr r1, =__init_array_end

ctor_loop:
    cmp r0, r1
    bhs call_kernel_main
    ldr r2, [r0], #4
    blx r2
    b ctor_loop

call_kernel_main:
    bl kernel_main

hang:
    b hang
