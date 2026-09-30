/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Defaults (used when neither DT nor settings provide a value) */
#define CIRQUE_DATA_MODE_DEFAULT       0  /* absolute */
#define CIRQUE_SENSITIVITY_DEFAULT     0  /* 1x */
#define CIRQUE_INVERT_X_DEFAULT        false
#define CIRQUE_INVERT_Y_DEFAULT        false
#define CIRQUE_SWAP_XY_DEFAULT         false
#define CIRQUE_ROTATE_DEGREES_DEFAULT  0

#define CIRQUE_TAP_MAX_MS_DEFAULT                250
#define CIRQUE_TAP_MAX_MOVEMENT_DEFAULT          200
#define CIRQUE_TAP_CLICK_MS_DEFAULT              30
#define CIRQUE_TAP_DRAG_TIMEOUT_MS_DEFAULT       350
#define CIRQUE_TAP_DRAG_MAX_MOVEMENT_DEFAULT     150
#define CIRQUE_PRIMARY_TAP_ENABLE_DEFAULT        true
#define CIRQUE_SECONDARY_TAP_ENABLE_DEFAULT      false
#define CIRQUE_AUX_TAP_ENABLE_DEFAULT            false
#define CIRQUE_SECONDARY_TAP_AREA_DEFAULT        0
#define CIRQUE_AUX_TAP_AREA_DEFAULT              0

#define CIRQUE_EDGE_MOTION_ENABLE_DEFAULT        false
#define CIRQUE_EDGE_MOTION_ZONE_DEFAULT          100
#define CIRQUE_EDGE_MOTION_SPEED_DEFAULT         5
#define CIRQUE_EDGE_MOTION_INTERVAL_MS_DEFAULT   50
#define CIRQUE_EDGE_MOTION_START_MS_DEFAULT      300

#define CIRQUE_RIGHT_EDGE_SCROLL_ENABLE_DEFAULT  false
#define CIRQUE_TOP_EDGE_SCROLL_ENABLE_DEFAULT    false
#define CIRQUE_SCROLL_ZONE_DEFAULT               80
#define CIRQUE_SCROLL_DIVISOR_DEFAULT            8
#define CIRQUE_INVERT_SCROLL_DEFAULT             false

#define CIRQUE_RELATIVE_MULTIPLIER_DEFAULT       1
#define CIRQUE_RELATIVE_DIVISOR_DEFAULT          1
#define CIRQUE_ABS_RELATIVE_MULTIPLIER_DEFAULT   1
#define CIRQUE_ABS_RELATIVE_DIVISOR_DEFAULT     1

#define CIRQUE_SLEEP_MODE_ENABLE_DEFAULT         true

struct cirque_runtime_state {
    /* data mode + sensitivity */
    uint8_t data_mode;
    uint8_t sensitivity;

    /* axis transforms */
    bool invert_x;
    bool invert_y;
    bool swap_xy;
    uint16_t rotate_degrees;

    /* tap behavior */
    bool primary_tap_enable;
    bool secondary_tap_enable;
    bool aux_tap_enable;
    uint16_t tap_max_ms;
    uint16_t tap_max_movement;
    uint16_t tap_click_ms;
    bool tap_drag_enable;
    uint16_t tap_drag_timeout_ms;
    uint16_t tap_drag_max_movement;
    uint16_t secondary_tap_area_width;
    uint16_t secondary_tap_area_height;
    uint16_t aux_tap_area_width;
    uint16_t aux_tap_area_height;

    /* edge motion */
    bool edge_motion_enable;
    uint16_t edge_motion_zone;
    uint16_t edge_motion_speed;
    uint16_t edge_motion_interval_ms;
    uint16_t edge_motion_start_ms;

    /* edge scroll */
    bool right_edge_scroll_enable;
    bool top_edge_scroll_enable;
    uint16_t scroll_zone;
    uint16_t scroll_divisor;
    bool invert_scroll;

    /* pointer scaling */
    uint32_t relative_multiplier;
    uint32_t relative_divisor;
    uint16_t absolute_relative_multiplier;
    uint16_t absolute_relative_divisor;

    /* power */
    bool sleep_mode_enable;
};

/* Getters (read-only, cheap) */
uint8_t  cirque_state_get_data_mode(const struct device *dev);
uint8_t  cirque_state_get_sensitivity(const struct device *dev);
bool     cirque_state_get_invert_x(const struct device *dev);
bool     cirque_state_get_invert_y(const struct device *dev);
bool     cirque_state_get_swap_xy(const struct device *dev);
uint16_t cirque_state_get_rotate_degrees(const struct device *dev);
bool     cirque_state_get_primary_tap_enable(const struct device *dev);
bool     cirque_state_get_secondary_tap_enable(const struct device *dev);
bool     cirque_state_get_aux_tap_enable(const struct device *dev);
uint16_t cirque_state_get_tap_max_ms(const struct device *dev);
uint16_t cirque_state_get_tap_max_movement(const struct device *dev);
uint16_t cirque_state_get_tap_click_ms(const struct device *dev);
bool     cirque_state_get_tap_drag_enable(const struct device *dev);
uint16_t cirque_state_get_tap_drag_timeout_ms(const struct device *dev);
uint16_t cirque_state_get_tap_drag_max_movement(const struct device *dev);
uint16_t cirque_state_get_secondary_tap_area_width(const struct device *dev);
uint16_t cirque_state_get_secondary_tap_area_height(const struct device *dev);
uint16_t cirque_state_get_aux_tap_area_width(const struct device *dev);
uint16_t cirque_state_get_aux_tap_area_height(const struct device *dev);
bool     cirque_state_get_edge_motion_enable(const struct device *dev);
uint16_t cirque_state_get_edge_motion_zone(const struct device *dev);
uint16_t cirque_state_get_edge_motion_speed(const struct device *dev);
uint16_t cirque_state_get_edge_motion_interval_ms(const struct device *dev);
uint16_t cirque_state_get_edge_motion_start_ms(const struct device *dev);
bool     cirque_state_get_right_edge_scroll_enable(const struct device *dev);
bool     cirque_state_get_top_edge_scroll_enable(const struct device *dev);
uint16_t cirque_state_get_scroll_zone(const struct device *dev);
uint16_t cirque_state_get_scroll_divisor(const struct device *dev);
bool     cirque_state_get_invert_scroll(const struct device *dev);
uint32_t cirque_state_get_relative_multiplier(const struct device *dev);
uint32_t cirque_state_get_relative_divisor(const struct device *dev);
uint16_t cirque_state_get_abs_relative_multiplier(const struct device *dev);
uint16_t cirque_state_get_abs_relative_divisor(const struct device *dev);
bool     cirque_state_get_sleep_mode_enable(const struct device *dev);

/* Setters (validate, write state, raise trackpad_status_changed) */
int cirque_state_set_data_mode(const struct device *dev, uint8_t mode);
int cirque_state_set_sensitivity(const struct device *dev, uint8_t s);
int cirque_state_set_invert_x(const struct device *dev, bool v);
int cirque_state_set_invert_y(const struct device *dev, bool v);
int cirque_state_set_swap_xy(const struct device *dev, bool v);
int cirque_state_set_rotate_degrees(const struct device *dev, uint16_t deg);
int cirque_state_set_primary_tap_enable(const struct device *dev, bool v);
int cirque_state_set_secondary_tap_enable(const struct device *dev, bool v);
int cirque_state_set_aux_tap_enable(const struct device *dev, bool v);
int cirque_state_set_tap_max_ms(const struct device *dev, uint16_t v);
int cirque_state_set_tap_max_movement(const struct device *dev, uint16_t v);
int cirque_state_set_tap_click_ms(const struct device *dev, uint16_t v);
int cirque_state_set_tap_drag_enable(const struct device *dev, bool v);
int cirque_state_set_tap_drag_timeout_ms(const struct device *dev, uint16_t v);
int cirque_state_set_tap_drag_max_movement(const struct device *dev, uint16_t v);
int cirque_state_set_secondary_tap_area_width(const struct device *dev, uint16_t v);
int cirque_state_set_secondary_tap_area_height(const struct device *dev, uint16_t v);
int cirque_state_set_aux_tap_area_width(const struct device *dev, uint16_t v);
int cirque_state_set_aux_tap_area_height(const struct device *dev, uint16_t v);
int cirque_state_set_edge_motion_enable(const struct device *dev, bool v);
int cirque_state_set_edge_motion_zone(const struct device *dev, uint16_t v);
int cirque_state_set_edge_motion_speed(const struct device *dev, uint16_t v);
int cirque_state_set_edge_motion_interval_ms(const struct device *dev, uint16_t v);
int cirque_state_set_edge_motion_start_ms(const struct device *dev, uint16_t v);
int cirque_state_set_right_edge_scroll_enable(const struct device *dev, bool v);
int cirque_state_set_top_edge_scroll_enable(const struct device *dev, bool v);
int cirque_state_set_scroll_zone(const struct device *dev, uint16_t v);
int cirque_state_set_scroll_divisor(const struct device *dev, uint16_t v);
int cirque_state_set_invert_scroll(const struct device *dev, bool v);
int cirque_state_set_relative_multiplier(const struct device *dev, uint32_t v);
int cirque_state_set_relative_divisor(const struct device *dev, uint32_t v);
int cirque_state_set_abs_relative_multiplier(const struct device *dev, uint16_t v);
int cirque_state_set_abs_relative_divisor(const struct device *dev, uint16_t v);
int cirque_state_set_sleep_mode_enable(const struct device *dev, bool v);

/* Bulk ops */
int cirque_state_load_defaults(const struct device *dev);
int cirque_state_load_from_dt(const struct device *dev);
int cirque_state_load_from_settings(const struct device *dev);
int cirque_state_apply_all(const struct device *dev);

/* Direct access for drivers that need a write-through cache */
struct cirque_runtime_state *cirque_state_get_struct(const struct device *dev);

#ifdef __cplusplus
}
#endif