/*
 * SPDX-License-Identifier: MIT
 *
 * Split-relay helper for cirque runtime state sync.
 *
 * When CONFIG_ZMK_SPLIT_RELAY_EVENT is enabled the central uses
 * cirque_relay_send_state() to ship the full runtime state to the peripheral
 * after every state-changing Studio RPC. The peripheral's handler applies the
 * 34 fields via cirque_state_set_*() and logs one deterministic line
 * confirming receipt (including the version, which the BLE test asserts).
 *
 * The relay carrier is a plain, packed C struct -- NOT protobuf. ZMK's relay
 * (`ZMK_RELAY_EVENT_*`, <zmk/event_manager.h>) memcpy()s the whole event
 * struct into the relay payload byte-for-byte, so both split halves must be
 * built from the same struct definition (same layout). Both halves also run
 * the same CPU architecture (nRF52 / bsim x86, both little-endian), so no
 * byte-swapping is needed; if you ever relay between different-endian halves,
 * serialize fields explicitly instead of memcpy'ing a struct.
 *
 * The 34 fields (source + version + 34 payload fields) fit in ~64 bytes, well
 * within CONFIG_ZMK_SPLIT_RELAY_EVENT_DATA_LEN (default 128).
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

/* Bump when the wire layout of struct cirque_relay_state changes. */
#define CIRQUE_RELAY_STATE_VERSION 1

/*
 * The relay carrier. `source` is required by the relay macros' loop guard
 * (ZMK_RELAY_EVENT_SOURCE_SELF on send; stamped to the sender index + 1 on
 * receive). The struct must fit CONFIG_ZMK_SPLIT_RELAY_EVENT_DATA_LEN (ZMK's
 * default 128 is plenty for these fields) and the identifier string below
 * must fit CONFIG_ZMK_SPLIT_RELAY_EVENT_TYPE_NAME_LEN (default 4); the relay
 * macros BUILD_ASSERT both.
 *
 * The payload mirrors struct cirque_runtime_state (include/zmk/cirque_state.h)
 * field-for-field. Keep it in sync with the getter/setter lists in
 * src/studio/cirque_state.c and the pack/unpack loops in
 * src/split/cirque_relay.c.
 */
struct cirque_relay_state {
    uint8_t source;
    uint8_t version;

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
} __packed;

ZMK_EVENT_DECLARE(cirque_relay_state);

/*
 * Central-side entry point. Called from the Studio RPC handler after applying
 * state in handle_set_state() and handle_reset(). Reads the current runtime
 * state from `dev`, packs all 34 fields into the carrier, and raises it so the
 * direction macro ships it to the peripheral(s). Only compiled (like this
 * whole helper) when ZMK's CONFIG_ZMK_SPLIT_RELAY_EVENT is enabled -- the
 * helper has no Kconfig of its own.
 */
void cirque_relay_send_state(const struct device *dev);
