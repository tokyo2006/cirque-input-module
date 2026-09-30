/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Persistent custom-settings backing for cirque_runtime_state. One
 * ZMK_CUSTOM_SETTING_DEFINE per runtime field (34 total) under the
 * "tokyo2006__cirque" namespace, plus the save/reset/load helpers that the
 * Studio RPC handler reaches through extern declarations in
 * src/studio/cirque_handler.c.
 *
 * The cormoran/zmk-feature-custom-settings fork only ships signed
 * ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32 for scalar values, so every numeric
 * field (struct U8/U16/U32) is stored as INT32 and re-cast on read/write.
 * Unsigned 32-bit values that exceed INT32_MAX (2^31-1) cannot be persisted;
 * none of the cirque defaults come close, and `relative_multiplier` /
 * `relative_divisor` are intended as small scaling factors in practice.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
#include <cormoran/zmk/custom_settings.h>
#endif

#include <zmk/cirque_settings.h>
#include <zmk/cirque_state.h>
#include <zmk/events/trackpad_status_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define CIRQUE_NS "tokyo2006__cirque"

#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)

/* --------------------------------------------------------------------------
 * ZMK_CUSTOM_SETTING_DEFINE entries
 *
 * Signature (9 args, from the cormoran fork — verbatim from
 * src/studio/cirque_handler.c:35-39):
 *
 *   ZMK_CUSTOM_SETTING_DEFINE(name, ns, key,
 *                             value_type, default,
 *                             confidentiality,
 *                             read_perm, write_perm,
 *                             constraint);
 *
 *   confidentiality: RPC_PUBLIC so the Studio web UI may list them
 *   read/write_perm: UNSECURE so the UI may both read and edit them
 *                    live without an unlock prompt
 *   constraint:      NO_CONSTRAINT — range checks live in the matching
 *                    cirque_state_set_* validator, which is the source of
 *                    truth for acceptable values
 *
 * Storage mapping:
 *   U8  / U16 / U32  -> ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32 (signed;
 *                      cast at the read/write boundary)
 *   BOOL             -> ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL
 *
 * Defaults mirror the CIRQUE_*_DEFAULT macros in include/zmk/cirque_state.h.
 * `tap_drag_enable` has no CIRQUE_*_DEFAULT macro — cirque_state_load_defaults
 * hard-codes it to false, so we match that here.
 * --------------------------------------------------------------------------
 */

/* data mode + sensitivity */
ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_data_mode,
                          CIRQUE_NS, "data_mode",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_DATA_MODE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_sensitivity,
                          CIRQUE_NS, "sensitivity",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_SENSITIVITY_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

/* axis transforms */
ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_invert_x,
                          CIRQUE_NS, "invert_x",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_INVERT_X_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_invert_y,
                          CIRQUE_NS, "invert_y",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_INVERT_Y_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_swap_xy,
                          CIRQUE_NS, "swap_xy",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_SWAP_XY_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_rotate_degrees,
                          CIRQUE_NS, "rotate_degrees",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_ROTATE_DEGREES_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

/* tap behavior */
ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_primary_tap_enable,
                          CIRQUE_NS, "primary_tap_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_PRIMARY_TAP_ENABLE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_secondary_tap_enable,
                          CIRQUE_NS, "secondary_tap_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_SECONDARY_TAP_ENABLE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_aux_tap_enable,
                          CIRQUE_NS, "aux_tap_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_AUX_TAP_ENABLE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_tap_max_ms,
                          CIRQUE_NS, "tap_max_ms",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_TAP_MAX_MS_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_tap_max_movement,
                          CIRQUE_NS, "tap_max_movement",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_TAP_MAX_MOVEMENT_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_tap_click_ms,
                          CIRQUE_NS, "tap_click_ms",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_TAP_CLICK_MS_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_tap_drag_enable,
                          CIRQUE_NS, "tap_drag_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(false),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_tap_drag_timeout_ms,
                          CIRQUE_NS, "tap_drag_timeout_ms",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_TAP_DRAG_TIMEOUT_MS_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_tap_drag_max_movement,
                          CIRQUE_NS, "tap_drag_max_movement",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_TAP_DRAG_MAX_MOVEMENT_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_secondary_tap_area_width,
                          CIRQUE_NS, "secondary_tap_area_width",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_SECONDARY_TAP_AREA_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_secondary_tap_area_height,
                          CIRQUE_NS, "secondary_tap_area_height",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_SECONDARY_TAP_AREA_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_aux_tap_area_width,
                          CIRQUE_NS, "aux_tap_area_width",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_AUX_TAP_AREA_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_aux_tap_area_height,
                          CIRQUE_NS, "aux_tap_area_height",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_AUX_TAP_AREA_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

/* edge motion */
ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_edge_motion_enable,
                          CIRQUE_NS, "edge_motion_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_EDGE_MOTION_ENABLE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_edge_motion_zone,
                          CIRQUE_NS, "edge_motion_zone",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_EDGE_MOTION_ZONE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_edge_motion_speed,
                          CIRQUE_NS, "edge_motion_speed",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_EDGE_MOTION_SPEED_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_edge_motion_interval_ms,
                          CIRQUE_NS, "edge_motion_interval_ms",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_EDGE_MOTION_INTERVAL_MS_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_edge_motion_start_ms,
                          CIRQUE_NS, "edge_motion_start_ms",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_EDGE_MOTION_START_MS_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

/* edge scroll */
ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_right_edge_scroll_enable,
                          CIRQUE_NS, "right_edge_scroll_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_RIGHT_EDGE_SCROLL_ENABLE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_top_edge_scroll_enable,
                          CIRQUE_NS, "top_edge_scroll_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_TOP_EDGE_SCROLL_ENABLE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_scroll_zone,
                          CIRQUE_NS, "scroll_zone",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_SCROLL_ZONE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_scroll_divisor,
                          CIRQUE_NS, "scroll_divisor",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_SCROLL_DIVISOR_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_invert_scroll,
                          CIRQUE_NS, "invert_scroll",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_INVERT_SCROLL_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

/* pointer scaling */
ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_relative_multiplier,
                          CIRQUE_NS, "relative_multiplier",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_RELATIVE_MULTIPLIER_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_relative_divisor,
                          CIRQUE_NS, "relative_divisor",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_RELATIVE_DIVISOR_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_absolute_relative_multiplier,
                          CIRQUE_NS, "absolute_relative_multiplier",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_ABS_RELATIVE_MULTIPLIER_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_absolute_relative_divisor,
                          CIRQUE_NS, "absolute_relative_divisor",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32,
                          ZMK_CUSTOM_SETTING_VALUE_INT32(CIRQUE_ABS_RELATIVE_DIVISOR_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

/* power */
ZMK_CUSTOM_SETTING_DEFINE(cirque_setting_sleep_mode_enable,
                          CIRQUE_NS, "sleep_mode_enable",
                          ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL,
                          ZMK_CUSTOM_SETTING_VALUE_BOOL(CIRQUE_SLEEP_MODE_ENABLE_DEFAULT),
                          ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
                          ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

/* --------------------------------------------------------------------------
 * save_all: walk every field, write its current runtime value with mode
 * PERSIST so the next settings_save commits it. Persist-mode writes also
 * stage the value in memory, which is what we want (the runtime value
 * equals what we just persisted; a subsequent settings_save is a no-op).
 * --------------------------------------------------------------------------
 */
int cirque_settings_save_all(const struct device *dev) {
    int rc;
    int first_rc = 0;

#define SAVE_INT(field, get_fn)                                                                    \
    do {                                                                                           \
        rc = zmk_custom_setting_set_int32(                                                         \
            &cirque_setting_##field, (int32_t)(get_fn(dev)),                                       \
            ZMK_CUSTOM_SETTING_WRITE_MODE_PERSIST);                                                \
        if (rc != 0 && first_rc == 0) {                                                            \
            first_rc = rc;                                                                         \
            LOG_WRN("save_all: %s -> %d", #field, rc);                                             \
        }                                                                                          \
    } while (0)

#define SAVE_BOOL(field, get_fn)                                                                   \
    do {                                                                                           \
        rc = zmk_custom_setting_set_bool(                                                          \
            &cirque_setting_##field, (get_fn(dev)),                                               \
            ZMK_CUSTOM_SETTING_WRITE_MODE_PERSIST);                                                \
        if (rc != 0 && first_rc == 0) {                                                            \
            first_rc = rc;                                                                         \
            LOG_WRN("save_all: %s -> %d", #field, rc);                                             \
        }                                                                                          \
    } while (0)

    SAVE_INT(data_mode,                          cirque_state_get_data_mode);
    SAVE_INT(sensitivity,                        cirque_state_get_sensitivity);
    SAVE_BOOL(invert_x,                          cirque_state_get_invert_x);
    SAVE_BOOL(invert_y,                          cirque_state_get_invert_y);
    SAVE_BOOL(swap_xy,                           cirque_state_get_swap_xy);
    SAVE_INT(rotate_degrees,                     cirque_state_get_rotate_degrees);
    SAVE_BOOL(primary_tap_enable,                cirque_state_get_primary_tap_enable);
    SAVE_BOOL(secondary_tap_enable,              cirque_state_get_secondary_tap_enable);
    SAVE_BOOL(aux_tap_enable,                    cirque_state_get_aux_tap_enable);
    SAVE_INT(tap_max_ms,                         cirque_state_get_tap_max_ms);
    SAVE_INT(tap_max_movement,                   cirque_state_get_tap_max_movement);
    SAVE_INT(tap_click_ms,                       cirque_state_get_tap_click_ms);
    SAVE_BOOL(tap_drag_enable,                   cirque_state_get_tap_drag_enable);
    SAVE_INT(tap_drag_timeout_ms,                cirque_state_get_tap_drag_timeout_ms);
    SAVE_INT(tap_drag_max_movement,              cirque_state_get_tap_drag_max_movement);
    SAVE_INT(secondary_tap_area_width,           cirque_state_get_secondary_tap_area_width);
    SAVE_INT(secondary_tap_area_height,          cirque_state_get_secondary_tap_area_height);
    SAVE_INT(aux_tap_area_width,                 cirque_state_get_aux_tap_area_width);
    SAVE_INT(aux_tap_area_height,                cirque_state_get_aux_tap_area_height);
    SAVE_BOOL(edge_motion_enable,                cirque_state_get_edge_motion_enable);
    SAVE_INT(edge_motion_zone,                   cirque_state_get_edge_motion_zone);
    SAVE_INT(edge_motion_speed,                  cirque_state_get_edge_motion_speed);
    SAVE_INT(edge_motion_interval_ms,            cirque_state_get_edge_motion_interval_ms);
    SAVE_INT(edge_motion_start_ms,               cirque_state_get_edge_motion_start_ms);
    SAVE_BOOL(right_edge_scroll_enable,          cirque_state_get_right_edge_scroll_enable);
    SAVE_BOOL(top_edge_scroll_enable,            cirque_state_get_top_edge_scroll_enable);
    SAVE_INT(scroll_zone,                        cirque_state_get_scroll_zone);
    SAVE_INT(scroll_divisor,                     cirque_state_get_scroll_divisor);
    SAVE_BOOL(invert_scroll,                     cirque_state_get_invert_scroll);
    SAVE_INT(relative_multiplier,                cirque_state_get_relative_multiplier);
    SAVE_INT(relative_divisor,                   cirque_state_get_relative_divisor);
    SAVE_INT(absolute_relative_multiplier,        cirque_state_get_abs_relative_multiplier);
    SAVE_INT(absolute_relative_divisor,           cirque_state_get_abs_relative_divisor);
    SAVE_BOOL(sleep_mode_enable,                 cirque_state_get_sleep_mode_enable);

#undef SAVE_INT
#undef SAVE_BOOL

    return first_rc;
}

/* reset_all: erase every persisted record under the cirque namespace.
 * Defaults stay in memory; cirque_state_load_defaults must be called
 * separately by the caller (the Studio RPC handler does that already).
 */
int cirque_settings_reset_all(const struct device *dev) {
    ARG_UNUSED(dev);
    uint32_t affected = 0;
    int rc = zmk_custom_settings_reset_scope(CIRQUE_NS, NULL, NULL, &affected);
    if (rc != 0) {
        LOG_WRN("reset_all: scope reset failed (%d), affected=%u", rc, affected);
    } else {
        LOG_DBG("reset_all: erased %u cirque settings", affected);
    }
    return rc;
}

/* load_cb: for each registered cirque setting, try to read it; if a value
 * is persisted (rc == 0), apply it to the runtime via cirque_state_set_*.
 * Unpersisted keys (rc == -ENOENT) are silently skipped — the runtime
 * keeps whatever cirque_state_load_defaults produced.
 *
 * Range / option validation is delegated to cirque_state_set_*; an invalid
 * persisted value is logged and skipped (it cannot abort the rest of the
 * load, which would leave the runtime half-initialized).
 */
int cirque_settings_load_cb(const struct device *dev) {
    int first_rc = 0;

#define LOAD_INT(field, set_fn)                                                                    \
    do {                                                                                           \
        int32_t v;                                                                                 \
        int rc = zmk_custom_setting_get_int32(&cirque_setting_##field, &v);                        \
        if (rc == 0) {                                                                             \
            rc = set_fn(dev, (uint32_t)v);                                                         \
            if (rc != 0) {                                                                         \
                LOG_WRN("load_cb: %s apply failed (%d)", #field, rc);                              \
                if (first_rc == 0) first_rc = rc;                                                  \
            }                                                                                      \
        } else if (rc != -ENOENT) {                                                                \
            LOG_WRN("load_cb: %s read failed (%d)", #field, rc);                                   \
            if (first_rc == 0) first_rc = rc;                                                      \
        }                                                                                          \
    } while (0)

#define LOAD_BOOL(field, set_fn)                                                                   \
    do {                                                                                           \
        bool v;                                                                                    \
        int rc = zmk_custom_setting_get_bool(&cirque_setting_##field, &v);                         \
        if (rc == 0) {                                                                             \
            rc = set_fn(dev, v);                                                                   \
            if (rc != 0) {                                                                         \
                LOG_WRN("load_cb: %s apply failed (%d)", #field, rc);                              \
                if (first_rc == 0) first_rc = rc;                                                  \
            }                                                                                      \
        } else if (rc != -ENOENT) {                                                                \
            LOG_WRN("load_cb: %s read failed (%d)", #field, rc);                                   \
            if (first_rc == 0) first_rc = rc;                                                      \
        }                                                                                          \
    } while (0)

    LOAD_INT(data_mode,                          cirque_state_set_data_mode);
    LOAD_INT(sensitivity,                        cirque_state_set_sensitivity);
    LOAD_BOOL(invert_x,                          cirque_state_set_invert_x);
    LOAD_BOOL(invert_y,                          cirque_state_set_invert_y);
    LOAD_BOOL(swap_xy,                           cirque_state_set_swap_xy);
    LOAD_INT(rotate_degrees,                     cirque_state_set_rotate_degrees);
    LOAD_BOOL(primary_tap_enable,                cirque_state_set_primary_tap_enable);
    LOAD_BOOL(secondary_tap_enable,              cirque_state_set_secondary_tap_enable);
    LOAD_BOOL(aux_tap_enable,                    cirque_state_set_aux_tap_enable);
    LOAD_INT(tap_max_ms,                         cirque_state_set_tap_max_ms);
    LOAD_INT(tap_max_movement,                   cirque_state_set_tap_max_movement);
    LOAD_INT(tap_click_ms,                       cirque_state_set_tap_click_ms);
    LOAD_BOOL(tap_drag_enable,                   cirque_state_set_tap_drag_enable);
    LOAD_INT(tap_drag_timeout_ms,                cirque_state_set_tap_drag_timeout_ms);
    LOAD_INT(tap_drag_max_movement,              cirque_state_set_tap_drag_max_movement);
    LOAD_INT(secondary_tap_area_width,           cirque_state_set_secondary_tap_area_width);
    LOAD_INT(secondary_tap_area_height,          cirque_state_set_secondary_tap_area_height);
    LOAD_INT(aux_tap_area_width,                 cirque_state_set_aux_tap_area_width);
    LOAD_INT(aux_tap_area_height,                cirque_state_set_aux_tap_area_height);
    LOAD_BOOL(edge_motion_enable,                cirque_state_set_edge_motion_enable);
    LOAD_INT(edge_motion_zone,                   cirque_state_set_edge_motion_zone);
    LOAD_INT(edge_motion_speed,                  cirque_state_set_edge_motion_speed);
    LOAD_INT(edge_motion_interval_ms,            cirque_state_set_edge_motion_interval_ms);
    LOAD_INT(edge_motion_start_ms,               cirque_state_set_edge_motion_start_ms);
    LOAD_BOOL(right_edge_scroll_enable,          cirque_state_set_right_edge_scroll_enable);
    LOAD_BOOL(top_edge_scroll_enable,            cirque_state_set_top_edge_scroll_enable);
    LOAD_INT(scroll_zone,                        cirque_state_set_scroll_zone);
    LOAD_INT(scroll_divisor,                     cirque_state_set_scroll_divisor);
    LOAD_BOOL(invert_scroll,                     cirque_state_set_invert_scroll);
    LOAD_INT(relative_multiplier,                cirque_state_set_relative_multiplier);
    LOAD_INT(relative_divisor,                   cirque_state_set_relative_divisor);
    LOAD_INT(absolute_relative_multiplier,        cirque_state_set_abs_relative_multiplier);
    LOAD_INT(absolute_relative_divisor,           cirque_state_set_abs_relative_divisor);
    LOAD_BOOL(sleep_mode_enable,                 cirque_state_set_sleep_mode_enable);

#undef LOAD_INT
#undef LOAD_BOOL

    return first_rc;
}

/* Wire load_cb to the "settings fully loaded" event the custom-settings
 * subsystem raises once after settings_load completes (see
 * zmk_custom_settings_initialized in <cormoran/zmk/custom_settings.h>).
 * That is the only safe moment to read settings at startup: earlier, a
 * SYS_INIT may race against settings_load and observe an empty default.
 */
static int cirque_settings_on_initialized(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);
    if (dev == NULL) {
        LOG_WRN("initialized event: no cirque,pinnacle2 device; skipping load");
        return ZMK_EV_EVENT_HANDLED;
    }
    (void)cirque_settings_load_cb(dev);
    return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(cirque_settings_initialized_listener, cirque_settings_on_initialized);
ZMK_SUBSCRIPTION(cirque_settings_initialized_listener, zmk_custom_settings_initialized);

#else /* !IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS) */

int cirque_settings_save_all(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

int cirque_settings_reset_all(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

int cirque_settings_load_cb(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

#endif /* IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS) */
