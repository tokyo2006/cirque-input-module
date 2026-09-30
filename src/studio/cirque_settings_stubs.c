/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Weak stubs for settings helpers referenced by the Studio RPC handler.
 * Task 12 will provide the real implementation; until then these allow the
 * build to link and the runtime to no-op gracefully (so the UI's "persist"
 * toggle is silently ignored rather than triggering a hard fault).
 */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

__attribute__((weak)) int cirque_settings_save_all(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

__attribute__((weak)) int cirque_settings_reset_all(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}