/*
 * main.cpp
 *
 * Bare-metal C++ firmware-style program for QEMU ARM virt machine.
 *
 * Target:
 *   qemu-system-arm -M virt -cpu cortex-a15
 *
 * Important:
 *   This does not run under Linux.
 *   There are no syscalls, no libc startup, no stdout, and no OS timer.
 *
 * Output is sent directly to QEMU's emulated PL011 UART.
 */

#include <stdint.h>
#include "logic.h"
#include "timer.h"
#include "uart.h"
#include "watchdog.h"
#include "boot_config.h"
#include "logger.h"

/* ------------------------------------------------------------
 * LED blinker state machine
 * ------------------------------------------------------------ */

class LedBlinker {
public:
    LedBlinker() : led_on_(false) {}

    void toggle() {
        led_on_ = next_led_state(led_on_);
        gpio_set_led(led_on_);

        if (led_on_) {
            Logger::write_line("LED ON");
        } else {
            Logger::write_line("LED OFF");
        }
    }

private:
    bool led_on_;
};

/* ------------------------------------------------------------
 * Application object
 * ------------------------------------------------------------ */

class Application {
public:
    explicit Application(const AppConfig &config)
        : config_(config) {}

    void init() {
        Watchdog::init(config_.watchdog_timeout_ms);
        Logger::write_line("");
        Logger::write_line("Bare-metal C++ application started on QEMU");
        Logger::write_line("Output is using the emulated PL011 UART");
        if (config_.has_watchdog_timeout_ms) {
            Logger::write_key_u32_ms("watchdog_timeout_ms_requested", config_.requested_watchdog_timeout_ms);
        }
        if (config_.has_loop_delay_ms) {
            Logger::write_key_u32_ms("loop_delay_ms_requested", config_.requested_loop_delay_ms);
        }
        Logger::write_key_u32_ms("watchdog_timeout_ms", config_.watchdog_timeout_ms);
        Logger::write_key_u32_ms("loop_delay_ms", config_.loop_delay_ms);
        gpio_set_led(false);

        if (config_.force_watchdog_timeout_once) {
            Logger::write_line("watchdog_test=forcing_timeout_on_next_pet");
            delay_ms(config_.watchdog_timeout_ms + 1U);
            Watchdog::pet();
            Logger::write_line("watchdog_test=unexpected_no_reset");
        }
    }

    void run_forever() {
        while (true) {
            Watchdog::pet();
            blinker_.toggle();

            delay_ms(config_.loop_delay_ms);
        }
    }

private:
    AppConfig config_;
    LedBlinker blinker_;
};

/* ------------------------------------------------------------
 * Called from startup.S after low-level CPU setup.
 * ------------------------------------------------------------ */

extern "C" void kernel_main(uint32_t dtb_addr) {
    const uintptr_t resolved_dtb_addr = dtb_resolve(static_cast<uintptr_t>(dtb_addr));

    Logger::write_hex_u32("dtb_addr", dtb_addr);
    Logger::write_hex_u32("dtb_addr_resolved", static_cast<uint32_t>(resolved_dtb_addr));

    if (resolved_dtb_addr != static_cast<uintptr_t>(dtb_addr)) {
        Logger::write_line("dtb_source=fallback_scan");
    } else {
        Logger::write_line("dtb_source=boot_register");
    }

    const AppConfig config = app_config_from_dtb(resolved_dtb_addr);
    Application app(config);
    app.init();
    app.run_forever();
}
