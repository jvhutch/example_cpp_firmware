#pragma once

#include <stdint.h>
#include <stddef.h>

struct AppConfig {
    uint32_t watchdog_timeout_ms;
    uint32_t loop_delay_ms;
};

AppConfig app_config_default();
AppConfig app_config_from_bootargs(const char *bootargs);
AppConfig app_config_from_dtb(uintptr_t dtb_addr);

uintptr_t dtb_resolve(uintptr_t boot_dtb_addr);
