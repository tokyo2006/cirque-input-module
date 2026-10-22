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
#include <zmk/cirque_settings.h>
#include <zmk/events/trackpad_status_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Driver-side hooks, provided by drivers/input/input_pinnacle.c whenever a
 * cirque,pinnacle2 node exists. Declared weak so this file links even in
 * builds without a trackpad device -- e.g. Studio RPC over USB in tests, or a
 * split half whose trackpad lives on the other hand. With no device the RPC
 * operates on a static fallback state so get/set/reset still round-trip; a
 * real keyboard always has a device, so behaviour there is unchanged. */
extern struct cirque_runtime_state *cirque_driver_get_state(const struct device *dev)
    __attribute__((weak));
extern void cirque_driver_apply_all(const struct device *dev) __attribute__((weak));

static void cirque_state_apply_defaults(struct cirque_runtime_state *s);

static struct cirque_runtime_state cirque_fallback_state;
static bool cirque_fallback_initialized;

struct cirque_runtime_state *cirque_state_get_struct(const struct device *dev) {
    if (dev == NULL || cirque_driver_get_state == NULL) {
        if (!cirque_fallback_initialized) {
            cirque_fallback_initialized = true;
            cirque_state_apply_defaults(&cirque_fallback_state);
        }
        return &cirque_fallback_state;
    }
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
static int validate_angle_tenths(uint16_t v) {
    /* 0.0 .. 90.0 degrees, tenths */
    return (v <= 900) ? 0 : -EINVAL;
}
static int validate_wedge_width(uint16_t v) {
    /* 5.0 .. 80.0 degrees, tenths */
    return (v >= 50 && v <= 800) ? 0 : -EINVAL;
}
static int validate_ring_percent(uint8_t v) {
    return (v <= 100) ? 0 : -EINVAL;
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
DEFINE_GETTER(uint16_t, secondary_tap_center_deg,  secondary_tap_center_deg)
DEFINE_GETTER(uint16_t, secondary_tap_width_deg,   secondary_tap_width_deg)
DEFINE_GETTER(uint16_t, wedge_min_slope,           wedge_min_slope_milli)
DEFINE_GETTER(uint16_t, wedge_max_slope,           wedge_max_slope_milli)
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
DEFINE_GETTER(uint8_t,  edge_scroll_ring_percent,  edge_scroll_ring_percent)
DEFINE_GETTER(uint16_t, active_range_x_min,        active_range_x_min)
DEFINE_GETTER(uint16_t, active_range_x_max,        active_range_x_max)
DEFINE_GETTER(uint16_t, active_range_y_min,        active_range_y_min)
DEFINE_GETTER(uint16_t, active_range_y_max,        active_range_y_max)
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
DEFINE_SETTER(uint8_t,  edge_scroll_ring_percent,  edge_scroll_ring_percent,  validate_ring_percent)
DEFINE_SETTER(bool,     invert_scroll,             invert_scroll,             validate_bool_any)
DEFINE_SETTER(uint32_t, relative_multiplier,       relative_multiplier,       validate_u32_any)
DEFINE_SETTER(uint32_t, relative_divisor,          relative_divisor,          validate_u32_any)
DEFINE_SETTER(uint16_t, abs_relative_multiplier,   absolute_relative_multiplier, validate_u16_any)
DEFINE_SETTER(uint16_t, abs_relative_divisor,      absolute_relative_divisor,    validate_u16_any)
DEFINE_SETTER(bool,     sleep_mode_enable,         sleep_mode_enable,         validate_bool_any)

/* tan(deg) * 1000 for 0.0 .. 90.0 degrees in 0.5 degree steps. A table keeps
 * libm (and ~280 KB of float code) out of the image: the wedge bounds only
 * need 0.5 degree resolution. */
static const uint16_t tan_milli_x1000[181] = {
    0, 9, 17, 26, 35, 44, 52, 61, 70, 79, 87,
    96, 105, 114, 123, 132, 141, 149, 158, 167, 176, 185,
    194, 203, 213, 222, 231, 240, 249, 259, 268, 277, 287,
    296, 306, 315, 325, 335, 344, 354, 364, 374, 384, 394,
    404, 414, 424, 435, 445, 456, 466, 477, 488, 499, 510,
    521, 532, 543, 554, 566, 577, 589, 601, 613, 625, 637,
    649, 662, 675, 687, 700, 713, 727, 740, 754, 767, 781,
    795, 810, 824, 839, 854, 869, 885, 900, 916, 933, 949,
    966, 983, 1000, 1018, 1036, 1054, 1072, 1091, 1111, 1130, 1150,
    1171, 1192, 1213, 1235, 1257, 1280, 1303, 1327, 1351, 1376, 1402,
    1428, 1455, 1483, 1511, 1540, 1570, 1600, 1632, 1664, 1698, 1732,
    1767, 1804, 1842, 1881, 1921, 1963, 2006, 2050, 2097, 2145, 2194,
    2246, 2300, 2356, 2414, 2475, 2539, 2605, 2675, 2747, 2824, 2904,
    2989, 3078, 3172, 3271, 3376, 3487, 3606, 3732, 3867, 4011, 4165,
    4331, 4511, 4705, 4915, 5145, 5396, 5671, 5976, 6314, 6691, 7115,
    7596, 8144, 8777, 9514, 10385, 11430, 12706, 14301, 16350, 19081, 22904,
    28636, 38188, 57290, 65535, 65535,
};

static uint16_t tan_milli(uint16_t deg_tenths) {
    uint16_t idx;

    if (deg_tenths > 900) {
        deg_tenths = 900;
    }
    /* nearest 0.5 degree entry */
    idx = (uint16_t)((deg_tenths + 2) / 5);
    if (idx > 180) {
        idx = 180;
    }
    return tan_milli_x1000[idx];
}

/* Recompute the cached integer slopes from the centre/width pair. Runs on
 * settings changes and at init, never in the sampling hot path. */
void cirque_state_refresh_wedge(const struct device *dev) {
    struct cirque_runtime_state *st = rt(dev);
    uint16_t half = st->secondary_tap_width_deg / 2;
    uint16_t lo = st->secondary_tap_center_deg > half
                      ? st->secondary_tap_center_deg - half
                      : 0;
    uint16_t hi = st->secondary_tap_center_deg + half;

    st->wedge_min_slope_milli = tan_milli(lo);
    st->wedge_max_slope_milli = tan_milli(hi);
}

int cirque_state_set_secondary_tap_center_deg(const struct device *dev, uint16_t v) {
    int rc = validate_angle_tenths(v);
    if (rc != 0) return rc;
    rt(dev)->secondary_tap_center_deg = v;
    cirque_state_refresh_wedge(dev);
    raise_trackpad_status_changed();
    return 0;
}

int cirque_state_set_secondary_tap_width_deg(const struct device *dev, uint16_t v) {
    int rc = validate_wedge_width(v);
    if (rc != 0) return rc;
    rt(dev)->secondary_tap_width_deg = v;
    cirque_state_refresh_wedge(dev);
    raise_trackpad_status_changed();
    return 0;
}

int cirque_state_load_defaults(const struct device *dev) {
    cirque_state_apply_defaults(cirque_state_get_struct(dev));
    return 0;
}

/* Assign every field its compile-time default. Shared by
 * cirque_state_load_defaults() and the lazy initialisation of the static
 * fallback state used when no trackpad device is present. */
static void cirque_state_apply_defaults(struct cirque_runtime_state *s) {
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
    s->secondary_tap_center_deg    = 0;
    s->secondary_tap_width_deg     = 0;
    s->wedge_min_slope_milli       = 0;
    s->wedge_max_slope_milli       = 0;
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
    s->edge_scroll_ring_percent    = 0;
    s->invert_scroll               = CIRQUE_INVERT_SCROLL_DEFAULT;
    s->relative_multiplier         = CIRQUE_RELATIVE_MULTIPLIER_DEFAULT;
    s->relative_divisor            = CIRQUE_RELATIVE_DIVISOR_DEFAULT;
    s->absolute_relative_multiplier= CIRQUE_ABS_RELATIVE_MULTIPLIER_DEFAULT;
    s->absolute_relative_divisor   = CIRQUE_ABS_RELATIVE_DIVISOR_DEFAULT;
    s->sleep_mode_enable           = CIRQUE_SLEEP_MODE_ENABLE_DEFAULT;
    s->active_range_x_min          = 0;
    s->active_range_x_max          = 0;
    s->active_range_y_min          = 0;
    s->active_range_y_max          = 0;
}

int cirque_state_load_from_dt(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

int cirque_state_load_from_settings(const struct device *dev) {
#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
    return cirque_settings_load_cb(dev);
#else
    ARG_UNUSED(dev);
    return 0;
#endif
}

int cirque_state_apply_all(const struct device *dev) {
    if (dev == NULL || cirque_driver_apply_all == NULL) {
        return 0;
    }
    cirque_driver_apply_all(dev);
    return 0;
}