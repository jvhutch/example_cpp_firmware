#include <iostream>
#include <cstring>
#include <string>

#include "../watchdog.h"
#include "../logger.h"

static int tests_failed = 0;

static uint32_t g_mock_freq_hz = 1000000U;
static uint64_t g_mock_now_ticks = 0U;
static int g_reset_calls = 0;
static int g_log_calls = 0;
static std::string g_last_log_line;

extern "C" uint32_t arch_timer_freq_hz(void) {
    return g_mock_freq_hz;
}

extern "C" uint64_t arch_timer_count(void) {
    return g_mock_now_ticks;
}

extern "C" void psci_system_reset(void) {
    ++g_reset_calls;
}

void Logger::write_line(const char *msg) {
    ++g_log_calls;
    g_last_log_line = (msg == nullptr) ? "" : msg;
}

static void reset_mocks() {
    g_mock_freq_hz = 1000000U;
    g_mock_now_ticks = 0U;
    g_reset_calls = 0;
    g_log_calls = 0;
    g_last_log_line.clear();
}

static void expect_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++tests_failed;
    }
}

static void test_pet_before_deadline_does_not_reset() {
    reset_mocks();

    g_mock_now_ticks = 100U;
    watchdog_init(2U); // timeout ticks: 2000 -> deadline 2100

    g_mock_now_ticks = 2099U;
    watchdog_pet();

    expect_true(g_reset_calls == 0, "watchdog should not reset before deadline");
    expect_true(g_log_calls == 0, "watchdog should not warn before deadline");
}

static void test_pet_on_deadline_resets_and_warns() {
    reset_mocks();

    g_mock_now_ticks = 500U;
    watchdog_init(1U); // timeout ticks: 1000 -> deadline 1500

    g_mock_now_ticks = 1500U;
    watchdog_pet();

    expect_true(g_reset_calls == 1, "watchdog should reset when now equals deadline");
    expect_true(g_log_calls == 1, "watchdog should log one warning on timeout");
    expect_true(g_last_log_line == "watchdog_timeout_warning=deadline_expired_resetting",
                "watchdog warning message should match expected text");
}

static void test_pet_after_deadline_resets_and_warns() {
    reset_mocks();

    g_mock_now_ticks = 0U;
    watchdog_init(1U); // deadline 1000

    g_mock_now_ticks = 2000U;
    watchdog_pet();

    expect_true(g_reset_calls == 1, "watchdog should reset when now is past deadline");
    expect_true(g_log_calls == 1, "watchdog should log warning when now is past deadline");
}

static void test_pet_advances_deadline() {
    reset_mocks();

    // init at T=0: deadline = 1000
    g_mock_now_ticks = 0U;
    watchdog_init(1U); // timeout_ms=1 -> 1000 ticks at 1MHz

    // pet at T=500 (before deadline): deadline should advance to 500+1000=1500
    g_mock_now_ticks = 500U;
    watchdog_pet();
    expect_true(g_reset_calls == 0, "early pet should not reset");

    // T=1000 would have expired the original deadline; the advanced deadline is 1500,
    // so no reset should occur.  No intermediate pet — don't move the deadline again.
    g_mock_now_ticks = 1000U;
    watchdog_pet();
    expect_true(g_reset_calls == 0, "pet at T=1000 should not reset (deadline advanced to 1500)");

    // T=1500: on the new deadline — deadline was set to 500+1000=1500 by the pet at T=500.
    // The pet at T=1000 set deadline to 1000+1000=2000, so we need to go past 2000.
    // Correct test: verify that without further petting the deadline at T=2000 triggers.
    g_mock_now_ticks = 2000U;
    watchdog_pet();
    expect_true(g_reset_calls == 1, "pet at T=2000 should reset (deadline is 2000 after pet at T=1000)");
}

int main() {
    test_pet_before_deadline_does_not_reset();
    test_pet_on_deadline_resets_and_warns();
    test_pet_after_deadline_resets_and_warns();
    test_pet_advances_deadline();

    if (tests_failed == 0) {
        std::cout << "All unit tests passed\n";
        return 0;
    }

    std::cerr << tests_failed << " test(s) failed\n";
    return 1;
}
