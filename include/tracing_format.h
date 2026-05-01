#pragma once

#include <stdint.h>
#include <stddef.h>

/*
 * Pure string-formatting helpers for tracing output.
 *
 * These are separated from tracing.cpp so they can be unit tested on the host
 * without any UART or MMIO dependencies.
 */

size_t trace_append_str(char *dst, size_t pos, size_t cap, const char *s);
size_t trace_append_hex_uintptr(char *dst, size_t pos, size_t cap, uintptr_t value);
