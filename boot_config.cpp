#include "boot_config.h"

namespace {

static constexpr uint32_t DEFAULT_WATCHDOG_MS = 2000U;
static constexpr uint32_t DEFAULT_LOOP_MS = 500U;
static constexpr uint32_t MIN_WATCHDOG_MS = 100U;
static constexpr uint32_t MAX_WATCHDOG_MS = 60000U;
static constexpr uint32_t MIN_LOOP_MS = 10U;
static constexpr uint32_t MAX_LOOP_MS = 10000U;

static constexpr uintptr_t RAM_BASE       = 0x40000000U;
static constexpr uintptr_t RAM_END        = 0x48000000U;
static constexpr uintptr_t FIXED_DTB_ADDR = 0x44000000U;

static constexpr uint32_t FDT_MAGIC = 0xD00DFEEDU;
static constexpr uint32_t FDT_BEGIN_NODE = 0x00000001U;
static constexpr uint32_t FDT_END_NODE = 0x00000002U;
static constexpr uint32_t FDT_PROP = 0x00000003U;
static constexpr uint32_t FDT_NOP = 0x00000004U;
static constexpr uint32_t FDT_END = 0x00000009U;

static uint32_t read_be32(const uint8_t *p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) |
           static_cast<uint32_t>(p[3]);
}

static size_t cstr_len(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

static bool cstr_eq(const char *a, const char *b) {
    size_t i = 0;
    while ((a[i] != '\0') && (b[i] != '\0')) {
        if (a[i] != b[i]) {
            return false;
        }
        ++i;
    }
    return (a[i] == '\0') && (b[i] == '\0');
}

static bool has_cstr_terminator(const char *s, size_t max_len) {
    for (size_t i = 0; i < max_len; ++i) {
        if (s[i] == '\0') {
            return true;
        }
    }
    return false;
}

static uint32_t clamp_u32(uint32_t value, uint32_t min_value, uint32_t max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static void normalize(AppConfig &cfg) {
    cfg.watchdog_timeout_ms = clamp_u32(cfg.watchdog_timeout_ms, MIN_WATCHDOG_MS, MAX_WATCHDOG_MS);
    cfg.loop_delay_ms = clamp_u32(cfg.loop_delay_ms, MIN_LOOP_MS, MAX_LOOP_MS);

    if (cfg.watchdog_timeout_ms <= cfg.loop_delay_ms) {
        cfg.watchdog_timeout_ms = cfg.loop_delay_ms * 2U;
        if (cfg.watchdog_timeout_ms > MAX_WATCHDOG_MS) {
            cfg.watchdog_timeout_ms = MAX_WATCHDOG_MS;
        }
    }
}

static bool parse_u32_token(const char *start, size_t len, uint32_t &out_value) {
    if (len == 0U) {
        return false;
    }

    uint64_t value = 0U;
    for (size_t i = 0; i < len; ++i) {
        const char c = start[i];
        if ((c < '0') || (c > '9')) {
            return false;
        }
        value = (value * 10U) + static_cast<uint32_t>(c - '0');
        if (value > 0xFFFFFFFFULL) {
            return false;
        }
    }

    out_value = static_cast<uint32_t>(value);
    return true;
}

static bool match_key_prefix(const char *token_start, size_t token_len, const char *key) {
    const size_t key_len = cstr_len(key);
    if (token_len <= key_len) {
        return false;
    }
    for (size_t i = 0; i < key_len; ++i) {
        if (token_start[i] != key[i]) {
            return false;
        }
    }
    return token_start[key_len] == '=';
}

static AppConfig parse_bootargs_bytes(const char *bootargs, size_t len) {
    AppConfig cfg = app_config_default();
    size_t pos = 0;

    while (pos < len) {
        while ((pos < len) && ((bootargs[pos] == ' ') || (bootargs[pos] == '\0'))) {
            ++pos;
        }
        if (pos >= len) {
            break;
        }

        const size_t token_start = pos;
        while ((pos < len) && (bootargs[pos] != ' ') && (bootargs[pos] != '\0')) {
            ++pos;
        }
        const size_t token_len = pos - token_start;
        const char *token = bootargs + token_start;

        if (match_key_prefix(token, token_len, "watchdog_timeout_ms")) {
            uint32_t parsed = 0U;
            const char *value_start = token + cstr_len("watchdog_timeout_ms") + 1U;
            const size_t value_len = token_len - cstr_len("watchdog_timeout_ms") - 1U;
            if (parse_u32_token(value_start, value_len, parsed)) {
                cfg.requested_watchdog_timeout_ms = parsed;
                cfg.has_watchdog_timeout_ms = true;
                cfg.watchdog_timeout_ms = parsed;
            }
        } else if (match_key_prefix(token, token_len, "loop_delay_ms")) {
            uint32_t parsed = 0U;
            const char *value_start = token + cstr_len("loop_delay_ms") + 1U;
            const size_t value_len = token_len - cstr_len("loop_delay_ms") - 1U;
            if (parse_u32_token(value_start, value_len, parsed)) {
                cfg.requested_loop_delay_ms = parsed;
                cfg.has_loop_delay_ms = true;
                cfg.loop_delay_ms = parsed;
            }
        } else if (match_key_prefix(token, token_len, "force_watchdog_timeout_once")) {
            uint32_t parsed = 0U;
            const char *value_start = token + cstr_len("force_watchdog_timeout_once") + 1U;
            const size_t value_len = token_len - cstr_len("force_watchdog_timeout_once") - 1U;
            if (parse_u32_token(value_start, value_len, parsed)) {
                cfg.has_force_watchdog_timeout_once = true;
                cfg.force_watchdog_timeout_once = (parsed != 0U);
            }
        }
    }

    normalize(cfg);
    return cfg;
}

static bool dtb_header_looks_valid(uintptr_t addr) {
    if ((addr < RAM_BASE) || ((addr + 40U) > RAM_END)) {
        return false;
    }
    const uint8_t *base = reinterpret_cast<const uint8_t *>(addr);
    const uint32_t magic      = read_be32(base + 0U);
    const uint32_t totalsize  = read_be32(base + 4U);
    const uint32_t off_struct = read_be32(base + 8U);
    const uint32_t off_strs   = read_be32(base + 12U);
    const uint32_t sz_strs    = read_be32(base + 32U);
    const uint32_t sz_struct  = read_be32(base + 36U);
    if (magic != FDT_MAGIC) {
        return false;
    }
    if ((totalsize < 40U) || ((addr + totalsize) > RAM_END)) {
        return false;
    }
    if ((off_struct > totalsize) || (off_strs > totalsize)) {
        return false;
    }
    if ((sz_struct > (totalsize - off_struct)) ||
        (sz_strs   > (totalsize - off_strs))) {
        return false;
    }
    return true;
}

static uintptr_t find_dtb_fallback() {
    for (uintptr_t addr = RAM_BASE; (addr + 40U) <= RAM_END; addr += 4U) {
        const uint8_t *p = reinterpret_cast<const uint8_t *>(addr);
        if (read_be32(p) != FDT_MAGIC) {
            continue;
        }
        if (dtb_header_looks_valid(addr)) {
            return addr;
        }
    }
    return 0U;
}

} // namespace

AppConfig app_config_default() {
    AppConfig cfg{};
    cfg.watchdog_timeout_ms = DEFAULT_WATCHDOG_MS;
    cfg.loop_delay_ms = DEFAULT_LOOP_MS;
    cfg.requested_watchdog_timeout_ms = DEFAULT_WATCHDOG_MS;
    cfg.requested_loop_delay_ms = DEFAULT_LOOP_MS;
    cfg.force_watchdog_timeout_once = false;
    cfg.has_watchdog_timeout_ms = false;
    cfg.has_loop_delay_ms = false;
    cfg.has_force_watchdog_timeout_once = false;
    return cfg;
}

AppConfig app_config_from_bootargs(const char *bootargs) {
    if (bootargs == nullptr) {
        return app_config_default();
    }
    return parse_bootargs_bytes(bootargs, cstr_len(bootargs));
}

uintptr_t dtb_resolve(uintptr_t boot_dtb_addr) {
    if (dtb_header_looks_valid(boot_dtb_addr)) {
        return boot_dtb_addr;
    }
    if (dtb_header_looks_valid(FIXED_DTB_ADDR)) {
        return FIXED_DTB_ADDR;
    }
    return find_dtb_fallback();
}

AppConfig app_config_from_dtb(uintptr_t dtb_addr) {
    const uint8_t *base = reinterpret_cast<const uint8_t *>(dtb_addr);
    AppConfig cfg = app_config_default();

    if (base == nullptr) {
        return cfg;
    }

    const uint32_t magic = read_be32(base + 0U);
    const uint32_t totalsize = read_be32(base + 4U);
    const uint32_t off_dt_struct = read_be32(base + 8U);
    const uint32_t off_dt_strings = read_be32(base + 12U);
    const uint32_t size_dt_strings = read_be32(base + 32U);
    const uint32_t size_dt_struct = read_be32(base + 36U);

    if (magic != FDT_MAGIC) {
        return cfg;
    }

    if ((off_dt_struct > totalsize) || (off_dt_strings > totalsize)) {
        return cfg;
    }
    if ((size_dt_struct > totalsize - off_dt_struct) || (size_dt_strings > totalsize - off_dt_strings)) {
        return cfg;
    }

    const uint8_t *struct_ptr = base + off_dt_struct;
    const uint8_t *struct_end = struct_ptr + size_dt_struct;
    const char *strings_ptr = reinterpret_cast<const char *>(base + off_dt_strings);
    const char *strings_end = strings_ptr + size_dt_strings;

    int32_t depth = 0;
    int32_t chosen_depth = -1;
    const uint8_t *p = struct_ptr;

    while ((p + 4U) <= struct_end) {
        const uint32_t token = read_be32(p);
        p += 4U;

        if (token == FDT_BEGIN_NODE) {
            const char *name = reinterpret_cast<const char *>(p);
            size_t name_len = 0U;
            while ((p + name_len) < struct_end) {
                if (name[name_len] == '\0') {
                    break;
                }
                ++name_len;
            }
            if ((p + name_len) >= struct_end) {
                return cfg;
            }

            p += ((name_len + 1U) + 3U) & ~3U;
            ++depth;

            if ((depth == 2) && cstr_eq(name, "chosen")) {
                chosen_depth = depth;
            }
            continue;
        }

        if (token == FDT_END_NODE) {
            if (depth == chosen_depth) {
                chosen_depth = -1;
            }
            if (depth > 0) {
                --depth;
            }
            continue;
        }

        if (token == FDT_PROP) {
            if ((p + 8U) > struct_end) {
                return cfg;
            }

            const uint32_t len = read_be32(p);
            const uint32_t nameoff = read_be32(p + 4U);
            p += 8U;

            if ((p + len) > struct_end) {
                return cfg;
            }

            if (chosen_depth == 2) {
                if (nameoff < size_dt_strings) {
                    const char *prop_name = strings_ptr + nameoff;
                    if ((prop_name < strings_end) && has_cstr_terminator(prop_name, static_cast<size_t>(strings_end - prop_name))) {
                        if (cstr_eq(prop_name, "bootargs")) {
                            return parse_bootargs_bytes(reinterpret_cast<const char *>(p), len);
                        }
                    }
                }
            }

            p += (len + 3U) & ~3U;
            continue;
        }

        if (token == FDT_NOP) {
            continue;
        }

        if (token == FDT_END) {
            return cfg;
        }

        return cfg;
    }

    return cfg;
}
