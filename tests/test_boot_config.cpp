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
    expect_true(!cfg.force_watchdog_timeout_once, "default force timeout flag should be false");
    expect_true(!cfg.has_watchdog_timeout_ms, "default watchdog timeout should not be marked explicit");
    expect_true(!cfg.has_loop_delay_ms, "default loop delay should not be marked explicit");
    expect_true(!cfg.has_force_watchdog_timeout_once, "default force timeout flag should not be marked explicit");
}

static void test_parse_values() {
    const AppConfig cfg = app_config_from_bootargs("watchdog_timeout_ms=3000 loop_delay_ms=250");
    expect_true(cfg.watchdog_timeout_ms == 3000U, "watchdog_timeout_ms should parse");
    expect_true(cfg.loop_delay_ms == 250U, "loop_delay_ms should parse");
    expect_true(cfg.requested_watchdog_timeout_ms == 3000U, "requested watchdog timeout should be preserved");
    expect_true(cfg.requested_loop_delay_ms == 250U, "requested loop delay should be preserved");
    expect_true(cfg.has_watchdog_timeout_ms, "watchdog timeout should be marked explicit");
    expect_true(cfg.has_loop_delay_ms, "loop delay should be marked explicit");
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

static void test_requested_values_survive_normalization() {
    const AppConfig cfg = app_config_from_bootargs("watchdog_timeout_ms=100 loop_delay_ms=1000");
    expect_true(cfg.requested_watchdog_timeout_ms == 100U, "requested watchdog timeout should match bootargs");
    expect_true(cfg.requested_loop_delay_ms == 1000U, "requested loop delay should match bootargs");
    expect_true(cfg.watchdog_timeout_ms == 2000U, "effective watchdog timeout should be normalized above loop delay");
    expect_true(cfg.loop_delay_ms == 1000U, "effective loop delay should remain requested value");
}

static void test_invalid_tokens_ignored() {
    const AppConfig cfg = app_config_from_bootargs("watchdog_timeout_ms=abc loop_delay_ms=123x other=7");
    expect_true(cfg.watchdog_timeout_ms == 2000U, "invalid watchdog token should keep default");
    expect_true(cfg.loop_delay_ms == 500U, "invalid loop token should keep default");
}

static void test_force_watchdog_timeout_flag() {
    const AppConfig cfg_true = app_config_from_bootargs("force_watchdog_timeout_once=1");
    expect_true(cfg_true.force_watchdog_timeout_once, "force timeout flag should parse true");
    expect_true(cfg_true.has_force_watchdog_timeout_once, "force timeout flag should be marked explicit");

    const AppConfig cfg_false = app_config_from_bootargs("force_watchdog_timeout_once=0");
    expect_true(!cfg_false.force_watchdog_timeout_once, "force timeout flag should parse false");
    expect_true(cfg_false.has_force_watchdog_timeout_once, "force timeout flag explicit should persist for zero");
}

static void test_force_flag_survives_normalization() {
    const AppConfig cfg = app_config_from_bootargs(
        "watchdog_timeout_ms=100 loop_delay_ms=1000 force_watchdog_timeout_once=1");

    expect_true(cfg.requested_watchdog_timeout_ms == 100U,
                "requested watchdog timeout should remain raw input");
    expect_true(cfg.requested_loop_delay_ms == 1000U,
                "requested loop delay should remain raw input");
    expect_true(cfg.watchdog_timeout_ms == 2000U,
                "effective watchdog timeout should still be normalized");
    expect_true(cfg.loop_delay_ms == 1000U,
                "effective loop delay should keep requested value");
    expect_true(cfg.force_watchdog_timeout_once,
                "force timeout flag should remain enabled after normalization");
    expect_true(cfg.has_force_watchdog_timeout_once,
                "force timeout explicit flag should remain set after normalization");
}

int main() {
    test_defaults();
    test_parse_values();
    test_clamps();
    test_relation_enforced();
    test_requested_values_survive_normalization();
    test_invalid_tokens_ignored();
    test_force_watchdog_timeout_flag();
    test_force_flag_survives_normalization();

    if (tests_failed == 0) {
        std::cout << "All unit tests passed\n";
        return 0;
    }

    std::cerr << tests_failed << " test(s) failed\n";
    return 1;
}
