#include <iostream>

#include "logic.h"

static int tests_failed = 0;

static void expect_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++tests_failed;
    }
}

int main() {
    expect_true(next_led_state(false) == true, "next_led_state(false) should be true");
    expect_true(next_led_state(true) == false, "next_led_state(true) should be false");

    expect_true(ticks_from_ms(1000000ULL, 0U) == 0ULL, "ticks_from_ms should return 0 for 0ms");
    expect_true(ticks_from_ms(1000000ULL, 500U) == 500000ULL, "1MHz for 500ms should be 500000 ticks");
    expect_true(ticks_from_ms(62500000ULL, 1U) == 62500ULL, "62.5MHz for 1ms should be 62500 ticks");

    if (tests_failed == 0) {
        std::cout << "All unit tests passed\n";
        return 0;
    }

    std::cerr << tests_failed << " test(s) failed\n";
    return 1;
}
