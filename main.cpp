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
#include <stddef.h>
#include "logic.h"
#include "timer.h"
#include "uart.h"
#include "watchdog.h"

/* ------------------------------------------------------------
 * Small fixed-size logger
 * ------------------------------------------------------------ */

class Logger {
public:
    static void write(const char *msg) {
        if (msg == nullptr) {
            return;
        }
        uart_write(msg, str_len(msg));
    }

    static void write_line(const char *msg) {
        if (msg == nullptr) {
            return;
        }
        uart_write_line(msg, str_len(msg));
    }

private:
    static size_t str_len(const char *s) {
        size_t len = 0;
        while (s[len] != '\0') {
            ++len;
        }
        return len;
    }
};

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
    void init() {
        watchdog_init(2000U);
        Logger::write_line("");
        Logger::write_line("Bare-metal C++ application started on QEMU");
        Logger::write_line("Output is using the emulated PL011 UART");
        gpio_set_led(false);
    }

    void run_forever() {
        while (true) {
            watchdog_pet();
            blinker_.toggle();

            delay_ms(500U);
        }
    }

private:
    LedBlinker blinker_;
};

/* ------------------------------------------------------------
 * Static application instance
 * ------------------------------------------------------------ */

static Application app;

/* ------------------------------------------------------------
 * Called from startup.S after low-level CPU setup.
 * ------------------------------------------------------------ */

extern "C" void kernel_main(void) {
    app.init();
    app.run_forever();
}




