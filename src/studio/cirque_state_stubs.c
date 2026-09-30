/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Temporary stubs for driver-side symbols referenced by cirque_state.c.
 * Task 6 (driver refactor) provides real implementations and deletes this file.
 */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <zmk/cirque_state.h>

struct cirque_runtime_state *cirque_driver_get_state(const struct device *dev) {
    ARG_UNUSED(dev);
    static struct cirque_runtime_state s;
    return &s;
}

void cirque_driver_apply_all(const struct device *dev) {
    ARG_UNUSED(dev);
}