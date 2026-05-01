#include <iostream>
#include <cstring>

#include "tracing_format.h"

static int tests_failed = 0;

static void expect_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++tests_failed;
    }
}

static void expect_str_eq(const char *actual, const char *expected, const char *message) {
    if (std::strcmp(actual, expected) != 0) {
        std::cerr << "FAIL: " << message << "\n";
        std::cerr << "  expected: \"" << expected << "\"\n";
        std::cerr << "  actual:   \"" << actual << "\"\n";
        ++tests_failed;
    }
}

// ---- trace_append_str -------------------------------------------------------

static void test_append_str_basic() {
    char buf[16] = {};
    size_t pos = trace_append_str(buf, 0, sizeof(buf), "hello");
    buf[pos] = '\0';
    expect_str_eq(buf, "hello", "trace_append_str basic copy");
    expect_true(pos == 5, "trace_append_str returns correct length");
}

static void test_append_str_chained() {
    char buf[16] = {};
    size_t pos = 0;
    pos = trace_append_str(buf, pos, sizeof(buf), "ab");
    pos = trace_append_str(buf, pos, sizeof(buf), "cd");
    buf[pos] = '\0';
    expect_str_eq(buf, "abcd", "trace_append_str chained appends");
}

static void test_append_str_capacity_limit() {
    char buf[4] = {};
    size_t pos = trace_append_str(buf, 0, 3, "hello"); // cap=3, no space for 'h','e','l' only
    expect_true(pos == 3, "trace_append_str stops at capacity");
    expect_true(buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l', "trace_append_str fills up to cap");
}

static void test_append_str_empty_string() {
    char buf[8] = {};
    size_t pos = trace_append_str(buf, 0, sizeof(buf), "");
    expect_true(pos == 0, "trace_append_str empty string leaves pos unchanged");
}

// ---- trace_append_hex_uintptr -----------------------------------------------

static void test_append_hex_zero() {
    char buf[32] = {};
    size_t pos = trace_append_hex_uintptr(buf, 0, sizeof(buf), 0U);
    buf[pos] = '\0';

    // On a 32-bit uintptr_t this produces "0x00000000", on 64-bit "0x0000000000000000"
    expect_true(buf[0] == '0' && buf[1] == 'x', "trace_append_hex_uintptr starts with 0x");
    for (size_t i = 2; i < pos; ++i) {
        expect_true(buf[i] == '0', "trace_append_hex_uintptr zero value has all-zero nibbles");
    }
}

static void test_append_hex_known_value() {
    char buf[32] = {};
    size_t pos = trace_append_hex_uintptr(buf, 0, sizeof(buf), static_cast<uintptr_t>(0xDEADBEEFU));
    buf[pos] = '\0';

    // The last 8 hex characters must represent 0xDEADBEEF regardless of pointer width
    expect_true(pos >= 10, "trace_append_hex_uintptr produces at least '0x' + 8 digits");
    const char *tail = buf + pos - 8;
    expect_str_eq(tail, "DEADBEEF", "trace_append_hex_uintptr lower 32 bits of 0xDEADBEEF");
}

static void test_append_hex_at_offset() {
    char buf[32] = {};
    buf[0] = 'X';
    size_t pos = trace_append_hex_uintptr(buf, 1, sizeof(buf), static_cast<uintptr_t>(0xABU));
    expect_true(buf[0] == 'X', "trace_append_hex_uintptr does not overwrite before pos");
    expect_true(buf[1] == '0' && buf[2] == 'x', "trace_append_hex_uintptr starts 0x at given offset");
    (void)pos;
}

// ---- full trace line format -------------------------------------------------

static void test_enter_line_format() {
    char line[96] = {};
    size_t pos = 0;

    pos = trace_append_str(line, pos, sizeof(line), "[I] -> fn=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), static_cast<uintptr_t>(0x40000000U));
    pos = trace_append_str(line, pos, sizeof(line), " from=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), static_cast<uintptr_t>(0x40001000U));
    pos = trace_append_str(line, pos, sizeof(line), "\n");
    line[pos] = '\0';

    // Verify prefix and newline terminator
    expect_true(std::strncmp(line, "[I] -> fn=0x", 12) == 0, "enter line starts with [I] -> fn=0x");
    expect_true(line[pos - 1] == '\n', "enter line ends with newline");
}

static void test_exit_line_format() {
    char line[96] = {};
    size_t pos = 0;

    pos = trace_append_str(line, pos, sizeof(line), "[I] <- fn=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), static_cast<uintptr_t>(0x40000000U));
    pos = trace_append_str(line, pos, sizeof(line), " from=");
    pos = trace_append_hex_uintptr(line, pos, sizeof(line), static_cast<uintptr_t>(0x40001000U));
    pos = trace_append_str(line, pos, sizeof(line), "\n");
    line[pos] = '\0';

    expect_true(std::strncmp(line, "[I] <- fn=0x", 12) == 0, "exit line starts with [I] <- fn=0x");
    expect_true(line[pos - 1] == '\n', "exit line ends with newline");
}

// -----------------------------------------------------------------------------

int main() {
    test_append_str_basic();
    test_append_str_chained();
    test_append_str_capacity_limit();
    test_append_str_empty_string();

    test_append_hex_zero();
    test_append_hex_known_value();
    test_append_hex_at_offset();

    test_enter_line_format();
    test_exit_line_format();

    if (tests_failed == 0) {
        std::cout << "All unit tests passed\n";
        return 0;
    }

    std::cerr << tests_failed << " test(s) failed\n";
    return 1;
}
