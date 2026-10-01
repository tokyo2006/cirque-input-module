/*
 * SPDX-License-Identifier: MIT
 *
 * Split-relay helper for cirque runtime state sync -- see
 * include/tokyo2006/cirque/cirque_relay.h for the overview.
 *
 * This file is compiled into BOTH split roles (central and peripheral),
 * independently of the Studio RPC subsystem (only the central runs Studio).
 * The role split below is by CONFIG_ZMK_SPLIT_ROLE_CENTRAL.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zmk/event_manager.h>
#include <zmk/cirque_state.h>
#include <tokyo2006/cirque/cirque_relay.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Event implementation for the relay carrier struct. */
ZMK_EVENT_IMPL(cirque_relay_state);

/*
 * Wire the relay carrier central -> peripheral. The direction macros are
 * self-role-gating (the wrong-direction one expands empty per role), and
 * HANDLE only fires when a relay frame with our identifier ("Trs") is actually
 * received -- a central never receives "Trs" (the peripheral never sends it),
 * so listing both here is safe on either role.
 */
ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(cirque_relay_state, Trs, source)
ZMK_RELAY_EVENT_HANDLE(cirque_relay_state, Trs, source)

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

/*
 * Peripheral side: the carrier re-raised locally by ZMK_RELAY_EVENT_HANDLE
 * after the split transport delivers the relay frame. Decode the packed struct
 * and apply all 34 fields via cirque_state_set_*() (no persist -- the central
 * already saved). Keep this light: it runs on the split relay-receive path
 * (the system work queue).
 */
static int cirque_relay_on_state(const zmk_event_t *eh) {
    const struct cirque_relay_state *ev = as_cirque_relay_state(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);
    if (dev == NULL) {
        LOG_WRN("No cirque,pinnacle2 device on peripheral; ignoring relayed state");
        return ZMK_EV_EVENT_HANDLED;
    }

    (void)cirque_state_set_data_mode(dev, ev->data_mode);
    (void)cirque_state_set_sensitivity(dev, ev->sensitivity);
    (void)cirque_state_set_invert_x(dev, ev->invert_x);
    (void)cirque_state_set_invert_y(dev, ev->invert_y);
    (void)cirque_state_set_swap_xy(dev, ev->swap_xy);
    (void)cirque_state_set_rotate_degrees(dev, ev->rotate_degrees);
    (void)cirque_state_set_primary_tap_enable(dev, ev->primary_tap_enable);
    (void)cirque_state_set_secondary_tap_enable(dev, ev->secondary_tap_enable);
    (void)cirque_state_set_aux_tap_enable(dev, ev->aux_tap_enable);
    (void)cirque_state_set_tap_max_ms(dev, ev->tap_max_ms);
    (void)cirque_state_set_tap_max_movement(dev, ev->tap_max_movement);
    (void)cirque_state_set_tap_click_ms(dev, ev->tap_click_ms);
    (void)cirque_state_set_tap_drag_enable(dev, ev->tap_drag_enable);
    (void)cirque_state_set_tap_drag_timeout_ms(dev, ev->tap_drag_timeout_ms);
    (void)cirque_state_set_tap_drag_max_movement(dev, ev->tap_drag_max_movement);
    (void)cirque_state_set_secondary_tap_area_width(dev, ev->secondary_tap_area_width);
    (void)cirque_state_set_secondary_tap_area_height(dev, ev->secondary_tap_area_height);
    (void)cirque_state_set_aux_tap_area_width(dev, ev->aux_tap_area_width);
    (void)cirque_state_set_aux_tap_area_height(dev, ev->aux_tap_area_height);
    (void)cirque_state_set_edge_motion_enable(dev, ev->edge_motion_enable);
    (void)cirque_state_set_edge_motion_zone(dev, ev->edge_motion_zone);
    (void)cirque_state_set_edge_motion_speed(dev, ev->edge_motion_speed);
    (void)cirque_state_set_edge_motion_interval_ms(dev, ev->edge_motion_interval_ms);
    (void)cirque_state_set_edge_motion_start_ms(dev, ev->edge_motion_start_ms);
    (void)cirque_state_set_right_edge_scroll_enable(dev, ev->right_edge_scroll_enable);
    (void)cirque_state_set_top_edge_scroll_enable(dev, ev->top_edge_scroll_enable);
    (void)cirque_state_set_scroll_zone(dev, ev->scroll_zone);
    (void)cirque_state_set_scroll_divisor(dev, ev->scroll_divisor);
    (void)cirque_state_set_invert_scroll(dev, ev->invert_scroll);
    (void)cirque_state_set_relative_multiplier(dev, ev->relative_multiplier);
    (void)cirque_state_set_relative_divisor(dev, ev->relative_divisor);
    (void)cirque_state_set_abs_relative_multiplier(dev, ev->absolute_relative_multiplier);
    (void)cirque_state_set_abs_relative_divisor(dev, ev->absolute_relative_divisor);
    (void)cirque_state_set_sleep_mode_enable(dev, ev->sleep_mode_enable);

    LOG_DBG("Peripheral applied relayed state (v%u)", ev->version);
    return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(cirque_relay_peripheral, cirque_relay_on_state);
ZMK_SUBSCRIPTION(cirque_relay_peripheral, cirque_relay_state);

#endif // !CONFIG_ZMK_SPLIT_ROLE_CENTRAL

/*
 * Raise the carrier with source = SELF. On a split central the
 * CENTRAL_TO_PERIPHERAL listener above ships it to the peripheral(s); the
 * carrier is a plain packed C struct (no protobuf) copied byte-for-byte into
 * the relay payload. Defined on both roles so the linker is happy regardless
 * of role, but in practice only the central (Studio RPC handler) calls it.
 */
void cirque_relay_send_state(const struct device *dev) {
    struct cirque_relay_state ev = {
        .source = ZMK_RELAY_EVENT_SOURCE_SELF,
        .version = CIRQUE_RELAY_STATE_VERSION,
    };

    ev.data_mode = cirque_state_get_data_mode(dev);
    ev.sensitivity = cirque_state_get_sensitivity(dev);
    ev.invert_x = cirque_state_get_invert_x(dev);
    ev.invert_y = cirque_state_get_invert_y(dev);
    ev.swap_xy = cirque_state_get_swap_xy(dev);
    ev.rotate_degrees = cirque_state_get_rotate_degrees(dev);
    ev.primary_tap_enable = cirque_state_get_primary_tap_enable(dev);
    ev.secondary_tap_enable = cirque_state_get_secondary_tap_enable(dev);
    ev.aux_tap_enable = cirque_state_get_aux_tap_enable(dev);
    ev.tap_max_ms = cirque_state_get_tap_max_ms(dev);
    ev.tap_max_movement = cirque_state_get_tap_max_movement(dev);
    ev.tap_click_ms = cirque_state_get_tap_click_ms(dev);
    ev.tap_drag_enable = cirque_state_get_tap_drag_enable(dev);
    ev.tap_drag_timeout_ms = cirque_state_get_tap_drag_timeout_ms(dev);
    ev.tap_drag_max_movement = cirque_state_get_tap_drag_max_movement(dev);
    ev.secondary_tap_area_width = cirque_state_get_secondary_tap_area_width(dev);
    ev.secondary_tap_area_height = cirque_state_get_secondary_tap_area_height(dev);
    ev.aux_tap_area_width = cirque_state_get_aux_tap_area_width(dev);
    ev.aux_tap_area_height = cirque_state_get_aux_tap_area_height(dev);
    ev.edge_motion_enable = cirque_state_get_edge_motion_enable(dev);
    ev.edge_motion_zone = cirque_state_get_edge_motion_zone(dev);
    ev.edge_motion_speed = cirque_state_get_edge_motion_speed(dev);
    ev.edge_motion_interval_ms = cirque_state_get_edge_motion_interval_ms(dev);
    ev.edge_motion_start_ms = cirque_state_get_edge_motion_start_ms(dev);
    ev.right_edge_scroll_enable = cirque_state_get_right_edge_scroll_enable(dev);
    ev.top_edge_scroll_enable = cirque_state_get_top_edge_scroll_enable(dev);
    ev.scroll_zone = cirque_state_get_scroll_zone(dev);
    ev.scroll_divisor = cirque_state_get_scroll_divisor(dev);
    ev.invert_scroll = cirque_state_get_invert_scroll(dev);
    ev.relative_multiplier = cirque_state_get_relative_multiplier(dev);
    ev.relative_divisor = cirque_state_get_relative_divisor(dev);
    ev.absolute_relative_multiplier = cirque_state_get_abs_relative_multiplier(dev);
    ev.absolute_relative_divisor = cirque_state_get_abs_relative_divisor(dev);
    ev.sleep_mode_enable = cirque_state_get_sleep_mode_enable(dev);

    raise_cirque_relay_state(ev);
}
