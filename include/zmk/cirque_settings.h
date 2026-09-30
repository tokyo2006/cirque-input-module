/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Persist every field of cirque_runtime_state to its custom-settings record
 * under the "tokyo2006__cirque" namespace. Called by handle_set_state when
 * the web UI sets `persist=true`; also safe to call directly after a batch
 * of state mutations.
 *
 * Always returns 0 in the no-CUSTOM_SETTINGS build; otherwise returns the
 * first non-zero zmk_custom_setting_write/save result it sees, or 0 on full
 * success.
 */
int cirque_settings_save_all(const struct device *dev);

/* Erase every persisted record under the "tokyo2006__cirque" namespace
 * (the in-memory defaults are not touched; call cirque_state_load_defaults
 * separately if you also want the runtime to revert). Called by
 * handle_reset when the web UI sets `factory_defaults=true`.
 *
 * Always returns 0 in the no-CUSTOM_SETTINGS build.
 */
int cirque_settings_reset_all(const struct device *dev);

/* Read every "tokyo2006__cirque" custom setting and apply it to the runtime
 * state via cirque_state_set_*. Unpersisted keys are silently skipped (the
 * runtime keeps whatever default cirque_state_load_defaults produced).
 *
 * Wired to the zmk_custom_settings_initialized event so it runs once at
 * boot after settings_load completes; can also be called directly (e.g.
 * from a future cirque_state_load_from_settings implementation).
 *
 * Always returns 0 in the no-CUSTOM_SETTINGS build.
 */
int cirque_settings_load_cb(const struct device *dev);

#ifdef __cplusplus
}
#endif
