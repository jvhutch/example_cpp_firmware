/*
 * linker.ld
 *
 * Linker script for QEMU ARM virt.
 *
 * QEMU's ARM virt machine loads the kernel image into RAM. For many
 * simple 32-bit ARM virt bare-metal examples, 0x40000000 is the RAM base.
 */

ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x40000000, LENGTH = 128M
}

SECTIONS
{
    . = ORIGIN(RAM);

    .text : ALIGN(4)
    {
        KEEP(*(.text._start))
        *(.text*)
        *(.rodata*)
    } > RAM

    .init_array : ALIGN(4)
    {
        __init_array_start = .;
        KEEP(*(.init_array*))
        __init_array_end = .;
    } > RAM

    .data : ALIGN(4)
    {
        *(.data*)
    } > RAM

    .bss : ALIGN(4)
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > RAM

    . = ALIGN(8);
    . = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;
    __stack_top__ = .;
}
