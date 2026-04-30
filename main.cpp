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

/* ------------------------------------------------------------
 * Small fixed-size logger
 * ------------------------------------------------------------ */

class Logger {
public:
    void write(const char *msg) {
        if (msg == nullptr) {
            return;
        }

        size_t len = 0;
        while (msg[len] != '\0') {
            ++len;
        }

        uart_write(msg, len);
    }

    void write_line(const char *msg) {
        if (msg == nullptr) {
            return;
        }

        size_t len = 0;
        while (msg[len] != '\0') {
            ++len;
        }

        uart_write_line(msg, len);
    }
};

/* ------------------------------------------------------------
 * LED blinker state machine
 * ------------------------------------------------------------ */

class LedBlinker {
public:
    explicit LedBlinker(Logger &logger)
        : logger_(logger),
          led_on_(false) {}

    void toggle() {
        led_on_ = next_led_state(led_on_);
        gpio_set_led(led_on_);

        if (led_on_) {
            logger_.write_line("LED ON");
        } else {
            logger_.write_line("LED OFF");
        }
    }

private:
    Logger &logger_;
    bool led_on_;
};

/* ------------------------------------------------------------
 * Application object
 * ------------------------------------------------------------ */

class Application {
public:
    Application()
        : logger_(),
          blinker_(logger_) {}

    void init() {
        logger_.write_line("");
        logger_.write_line("Bare-metal C++ application started on QEMU");
        logger_.write_line("Output is using the emulated PL011 UART");
        gpio_set_led(false);
    }

    void run_forever() {
        while (true) {
            blinker_.toggle();

            delay_ms(500U);
        }
    }

private:
    Logger logger_;
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

/* ------------------------------------------------------------
 * Minimal C++ runtime hooks
 * ------------------------------------------------------------
 *
 * Because this is freestanding bare-metal C++, there is no normal hosted
 * runtime. These handlers avoid unresolved references if the compiler emits
 * calls for pure virtual handling or stack protector failure.
 */

extern "C" void __cxa_pure_virtual(void) {
    while (true) {
        /* trap */
    }
}

extern "C" void __stack_chk_fail(void) {
    while (true) {
        /* trap */
    }
}
