#include <iostream>

#include "../boot_config.h"

static int tests_failed = 0;

static void expect_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++tests_failed;
    }
}

static void test_defaults() {
    const AppConfig cfg = app_config_from_bootargs("");
    expect_true(cfg.watchdog_timeout_ms == 2000U, "default watchdog timeout should be 2000");
    expect_true(cfg.loop_delay_ms == 500U, "default loop delay should be 500");
}

static void test_parse_values() {
    const AppConfig cfg = app_config_from_bootargs("watchdog_timeout_ms=3000 loop_delay_ms=250");
    expect_true(cfg.watchdog_timeout_ms == 3000U, "watchdog_timeout_ms should parse");
    expect_true(cfg.loop_delay_ms == 250U, "loop_delay_ms should parse");
}

static void test_clamps() {
    const AppConfig cfg = app_config_from_bootargs("watchdog_timeout_ms=1 loop_delay_ms=1");
    expect_true(cfg.watchdog_timeout_ms >= 100U, "watchdog timeout should be clamped to min");
    expect_true(cfg.loop_delay_ms >= 10U, "loop delay should be clamped to min");
}

static void test_relation_enforced() {
    const AppConfig cfg = app_config_from_bootargs("watchdog_timeout_ms=100 loop_delay_ms=2000");
    expect_true(cfg.watchdog_timeout_ms > cfg.loop_delay_ms, "watchdog timeout should be greater than loop delay");
}

static void test_invalid_tokens_ignored() {
    const AppConfig cfg = app_config_from_bootargs("watchdog_timeout_ms=abc loop_delay_ms=123x other=7");
    expect_true(cfg.watchdog_timeout_ms == 2000U, "invalid watchdog token should keep default");
    expect_true(cfg.loop_delay_ms == 500U, "invalid loop token should keep default");
}

int main() {
    test_defaults();
    test_parse_values();
    test_clamps();
    test_relation_enforced();
    test_invalid_tokens_ignored();

    if (tests_failed == 0) {
        std::cout << "All unit tests passed\n";
        return 0;
    }

    std::cerr << tests_failed << " test(s) failed\n";
    return 1;
}
