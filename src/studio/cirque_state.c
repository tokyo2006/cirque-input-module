/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/cirque_state.h>
#include <zmk/events/trackpad_status_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct cirque_runtime_state *cirque_state_get_struct(const struct device *dev) {
    extern struct cirque_runtime_state *cirque_driver_get_state(const struct device *dev);
    return cirque_driver_get_state(dev);
}

static inline struct cirque_runtime_state *rt(const struct device *dev) {
    return cirque_state_get_struct(dev);
}

#define DEFINE_GETTER(T, name, field)                                   \
    T cirque_state_get_##name(const struct device *dev) {               \
        return rt(dev)->field;                                          \
    }

#define DEFINE_SETTER(T, name, field, validate_fn)                      \
    int cirque_state_set_##name(const struct device *dev, T v) {        \
        int rc = validate_fn(v);                                        \
        if (rc != 0) return rc;                                         \
        rt(dev)->field = v;                                             \
        raise_trackpad_status_changed();                                \
        return 0;                                                       \
    }

static int validate_u8_any(uint8_t v)  { (void)v; return 0; }
static int validate_u16_any(uint16_t v) { (void)v; return 0; }
static int validate_u32_any(uint32_t v) { (void)v; return 0; }
static int validate_bool_any(bool v)    { (void)v; return 0; }
static int validate_rotate(uint16_t v) {
    return (v == 0 || v == 90 || v == 180 || v == 270) ? 0 : -EINVAL;
}
static int validate_scroll_divisor(uint16_t v) {
    return (v >= 1 && v <= 65535) ? 0 : -EINVAL;
}

DEFINE_GETTER(uint8_t,  data_mode,                 data_mode)
DEFINE_GETTER(uint8_t,  sensitivity,               sensitivity)
DEFINE_GETTER(bool,     invert_x,                  invert_x)
DEFINE_GETTER(bool,     invert_y,                  invert_y)
DEFINE_GETTER(bool,     swap_xy,                   swap_xy)
DEFINE_GETTER(uint16_t, rotate_degrees,            rotate_degrees)
DEFINE_GETTER(bool,     primary_tap_enable,        primary_tap_enable)
DEFINE_GETTER(bool,     secondary_tap_enable,      secondary_tap_enable)
DEFINE_GETTER(bool,     aux_tap_enable,            aux_tap_enable)
DEFINE_GETTER(uint16_t, tap_max_ms,                tap_max_ms)
DEFINE_GETTER(uint16_t, tap_max_movement,          tap_max_movement)
DEFINE_GETTER(uint16_t, tap_click_ms,              tap_click_ms)
DEFINE_GETTER(bool,     tap_drag_enable,           tap_drag_enable)
DEFINE_GETTER(uint16_t, tap_drag_timeout_ms,       tap_drag_timeout_ms)
DEFINE_GETTER(uint16_t, tap_drag_max_movement,     tap_drag_max_movement)
DEFINE_GETTER(uint16_t, secondary_tap_area_width,  secondary_tap_area_width)
DEFINE_GETTER(uint16_t, secondary_tap_area_height, secondary_tap_area_height)
DEFINE_GETTER(uint16_t, aux_tap_area_width,        aux_tap_area_width)
DEFINE_GETTER(uint16_t, aux_tap_area_height,       aux_tap_area_height)
DEFINE_GETTER(bool,     edge_motion_enable,        edge_motion_enable)
DEFINE_GETTER(uint16_t, edge_motion_zone,          edge_motion_zone)
DEFINE_GETTER(uint16_t, edge_motion_speed,         edge_motion_speed)
DEFINE_GETTER(uint16_t, edge_motion_interval_ms,   edge_motion_interval_ms)
DEFINE_GETTER(uint16_t, edge_motion_start_ms,      edge_motion_start_ms)
DEFINE_GETTER(bool,     right_edge_scroll_enable,  right_edge_scroll_enable)
DEFINE_GETTER(bool,     top_edge_scroll_enable,    top_edge_scroll_enable)
DEFINE_GETTER(uint16_t, scroll_zone,               scroll_zone)
DEFINE_GETTER(uint16_t, scroll_divisor,            scroll_divisor)
DEFINE_GETTER(bool,     invert_scroll,             invert_scroll)
DEFINE_GETTER(uint32_t, relative_multiplier,       relative_multiplier)
DEFINE_GETTER(uint32_t, relative_divisor,          relative_divisor)
DEFINE_GETTER(uint16_t, abs_relative_multiplier,   absolute_relative_multiplier)
DEFINE_GETTER(uint16_t, abs_relative_divisor,      absolute_relative_divisor)
DEFINE_GETTER(bool,     sleep_mode_enable,         sleep_mode_enable)

DEFINE_SETTER(uint8_t,  data_mode,                 data_mode,                 validate_u8_any)
DEFINE_SETTER(uint8_t,  sensitivity,               sensitivity,               validate_u8_any)
DEFINE_SETTER(bool,     invert_x,                  invert_x,                  validate_bool_any)
DEFINE_SETTER(bool,     invert_y,                  invert_y,                  validate_bool_any)
DEFINE_SETTER(bool,     swap_xy,                   swap_xy,                   validate_bool_any)
DEFINE_SETTER(uint16_t, rotate_degrees,            rotate_degrees,            validate_rotate)
DEFINE_SETTER(bool,     primary_tap_enable,        primary_tap_enable,        validate_bool_any)
DEFINE_SETTER(bool,     secondary_tap_enable,      secondary_tap_enable,      validate_bool_any)
DEFINE_SETTER(bool,     aux_tap_enable,            aux_tap_enable,            validate_bool_any)
DEFINE_SETTER(uint16_t, tap_max_ms,                tap_max_ms,                validate_u16_any)
DEFINE_SETTER(uint16_t, tap_max_movement,          tap_max_movement,          validate_u16_any)
DEFINE_SETTER(uint16_t, tap_click_ms,              tap_click_ms,              validate_u16_any)
DEFINE_SETTER(bool,     tap_drag_enable,           tap_drag_enable,           validate_bool_any)
DEFINE_SETTER(uint16_t, tap_drag_timeout_ms,       tap_drag_timeout_ms,       validate_u16_any)
DEFINE_SETTER(uint16_t, tap_drag_max_movement,     tap_drag_max_movement,     validate_u16_any)
DEFINE_SETTER(uint16_t, secondary_tap_area_width,  secondary_tap_area_width,  validate_u16_any)
DEFINE_SETTER(uint16_t, secondary_tap_area_height, secondary_tap_area_height, validate_u16_any)
DEFINE_SETTER(uint16_t, aux_tap_area_width,        aux_tap_area_width,        validate_u16_any)
DEFINE_SETTER(uint16_t, aux_tap_area_height,       aux_tap_area_height,       validate_u16_any)
DEFINE_SETTER(bool,     edge_motion_enable,        edge_motion_enable,        validate_bool_any)
DEFINE_SETTER(uint16_t, edge_motion_zone,          edge_motion_zone,          validate_u16_any)
DEFINE_SETTER(uint16_t, edge_motion_speed,         edge_motion_speed,         validate_u16_any)
DEFINE_SETTER(uint16_t, edge_motion_interval_ms,   edge_motion_interval_ms,   validate_u16_any)
DEFINE_SETTER(uint16_t, edge_motion_start_ms,      edge_motion_start_ms,      validate_u16_any)
DEFINE_SETTER(bool,     right_edge_scroll_enable,  right_edge_scroll_enable,  validate_bool_any)
DEFINE_SETTER(bool,     top_edge_scroll_enable,    top_edge_scroll_enable,    validate_bool_any)
DEFINE_SETTER(uint16_t, scroll_zone,               scroll_zone,               validate_u16_any)
DEFINE_SETTER(uint16_t, scroll_divisor,            scroll_divisor,            validate_scroll_divisor)
DEFINE_SETTER(bool,     invert_scroll,             invert_scroll,             validate_bool_any)
DEFINE_SETTER(uint32_t, relative_multiplier,       relative_multiplier,       validate_u32_any)
DEFINE_SETTER(uint32_t, relative_divisor,          relative_divisor,          validate_u32_any)
DEFINE_SETTER(uint16_t, abs_relative_multiplier,   absolute_relative_multiplier, validate_u16_any)
DEFINE_SETTER(uint16_t, abs_relative_divisor,      absolute_relative_divisor,    validate_u16_any)
DEFINE_SETTER(bool,     sleep_mode_enable,         sleep_mode_enable,         validate_bool_any)

int cirque_state_load_defaults(const struct device *dev) {
    struct cirque_runtime_state *s = rt(dev);
    s->data_mode                   = CIRQUE_DATA_MODE_DEFAULT;
    s->sensitivity                 = CIRQUE_SENSITIVITY_DEFAULT;
    s->invert_x                    = CIRQUE_INVERT_X_DEFAULT;
    s->invert_y                    = CIRQUE_INVERT_Y_DEFAULT;
    s->swap_xy                     = CIRQUE_SWAP_XY_DEFAULT;
    s->rotate_degrees              = CIRQUE_ROTATE_DEGREES_DEFAULT;
    s->primary_tap_enable          = CIRQUE_PRIMARY_TAP_ENABLE_DEFAULT;
    s->secondary_tap_enable        = CIRQUE_SECONDARY_TAP_ENABLE_DEFAULT;
    s->aux_tap_enable              = CIRQUE_AUX_TAP_ENABLE_DEFAULT;
    s->tap_max_ms                  = CIRQUE_TAP_MAX_MS_DEFAULT;
    s->tap_max_movement            = CIRQUE_TAP_MAX_MOVEMENT_DEFAULT;
    s->tap_click_ms                = CIRQUE_TAP_CLICK_MS_DEFAULT;
    s->tap_drag_enable             = false;
    s->tap_drag_timeout_ms         = CIRQUE_TAP_DRAG_TIMEOUT_MS_DEFAULT;
    s->tap_drag_max_movement       = CIRQUE_TAP_DRAG_MAX_MOVEMENT_DEFAULT;
    s->secondary_tap_area_width    = CIRQUE_SECONDARY_TAP_AREA_DEFAULT;
    s->secondary_tap_area_height   = CIRQUE_SECONDARY_TAP_AREA_DEFAULT;
    s->aux_tap_area_width          = CIRQUE_AUX_TAP_AREA_DEFAULT;
    s->aux_tap_area_height         = CIRQUE_AUX_TAP_AREA_DEFAULT;
    s->edge_motion_enable          = CIRQUE_EDGE_MOTION_ENABLE_DEFAULT;
    s->edge_motion_zone            = CIRQUE_EDGE_MOTION_ZONE_DEFAULT;
    s->edge_motion_speed           = CIRQUE_EDGE_MOTION_SPEED_DEFAULT;
    s->edge_motion_interval_ms     = CIRQUE_EDGE_MOTION_INTERVAL_MS_DEFAULT;
    s->edge_motion_start_ms        = CIRQUE_EDGE_MOTION_START_MS_DEFAULT;
    s->right_edge_scroll_enable    = CIRQUE_RIGHT_EDGE_SCROLL_ENABLE_DEFAULT;
    s->top_edge_scroll_enable      = CIRQUE_TOP_EDGE_SCROLL_ENABLE_DEFAULT;
    s->scroll_zone                 = CIRQUE_SCROLL_ZONE_DEFAULT;
    s->scroll_divisor              = CIRQUE_SCROLL_DIVISOR_DEFAULT;
    s->invert_scroll               = CIRQUE_INVERT_SCROLL_DEFAULT;
    s->relative_multiplier         = CIRQUE_RELATIVE_MULTIPLIER_DEFAULT;
    s->relative_divisor            = CIRQUE_RELATIVE_DIVISOR_DEFAULT;
    s->absolute_relative_multiplier= CIRQUE_ABS_RELATIVE_MULTIPLIER_DEFAULT;
    s->absolute_relative_divisor   = CIRQUE_ABS_RELATIVE_DIVISOR_DEFAULT;
    s->sleep_mode_enable           = CIRQUE_SLEEP_MODE_ENABLE_DEFAULT;
    return 0;
}

int cirque_state_load_from_dt(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

int cirque_state_load_from_settings(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

int cirque_state_apply_all(const struct device *dev) {
    extern void cirque_driver_apply_all(const struct device *dev);
    cirque_driver_apply_all(dev);
    return 0;
}