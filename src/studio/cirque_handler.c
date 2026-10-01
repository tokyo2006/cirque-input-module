#include <errno.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zmk/studio/custom.h>
#include <tokyo2006/cirque/cirque.pb.h>

#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
#include <cormoran/zmk/custom_settings.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
#include <tokyo2006/cirque/cirque_relay.h>
#endif

#include <zmk/cirque_settings.h>
#include <zmk/drag_scroll.h>
#include <zmk/pointing_speed.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_rpc_custom_subsystem_meta cirque_subsystem_meta = {
    ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS("https://tokyo2006.github.io/cirque-input-module/"),
    // Unsecured is suggested by default to avoid unlocking in un-reliable
    // environments.
    // The web template already implements the unlock prompt/retry flow (see
    // web/src/App.tsx), so switching this to ZMK_STUDIO_RPC_HANDLER_SECURED
    // requires no web changes.
    .security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

ZMK_RPC_CUSTOM_SUBSYSTEM(tokyo2006__cirque, &cirque_subsystem_meta, cirque_rpc_handle_request);

ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(tokyo2006__cirque, tokyo2006_cirque_Response);

static int handle_get_state(const tokyo2006_cirque_GetStateRequest *req,
                            tokyo2006_cirque_Response *resp);
static int handle_set_state(const tokyo2006_cirque_SetStateRequest *req,
                            tokyo2006_cirque_Response *resp);
static int handle_reset(const tokyo2006_cirque_ResetRequest *req,
                        tokyo2006_cirque_Response *resp);

static bool cirque_rpc_handle_request(const zmk_custom_CallRequest *raw_request,
                                        pb_callback_t *encode_response) {
    tokyo2006_cirque_Response *resp =
        ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER_ALLOCATE(tokyo2006__cirque, encode_response);

    tokyo2006_cirque_Request req = tokyo2006_cirque_Request_init_zero;

    pb_istream_t req_stream =
        pb_istream_from_buffer(raw_request->payload.bytes, raw_request->payload.size);
    if (!pb_decode(&req_stream, tokyo2006_cirque_Request_fields, &req)) {
        LOG_WRN("Failed to decode cirque request: %s", PB_GET_ERROR(&req_stream));
        tokyo2006_cirque_ErrorResponse err = tokyo2006_cirque_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Failed to decode request");
        resp->which_response_type = tokyo2006_cirque_Response_error_tag;
        resp->response_type.error = err;
        return true;
    }

    int rc = 0;
    switch (req.which_request_type) {
    case tokyo2006_cirque_Request_get_state_tag:
        rc = handle_get_state(&req.request_type.get_state, resp);
        break;
    case tokyo2006_cirque_Request_set_state_tag:
        rc = handle_set_state(&req.request_type.set_state, resp);
        break;
    case tokyo2006_cirque_Request_reset_tag:
        rc = handle_reset(&req.request_type.reset, resp);
        break;
    default:
        LOG_WRN("Unsupported cirque request type: %d", req.which_request_type);
        rc = -1;
    }

    if (rc != 0 && resp->which_response_type != tokyo2006_cirque_Response_error_tag) {
        tokyo2006_cirque_ErrorResponse err = tokyo2006_cirque_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Failed to process request");
        resp->which_response_type = tokyo2006_cirque_Response_error_tag;
        resp->response_type.error = err;
    }
    return true;
}

static int handle_get_state(const tokyo2006_cirque_GetStateRequest *req,
                            tokyo2006_cirque_Response *resp) {
    ARG_UNUSED(req);

    /* dev is NULL when no cirque,pinnacle2 node exists (e.g. tests, or a split
     * half whose trackpad is on the other hand). The state layer falls back to
     * a static store so the RPC still round-trips. */
    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);

    tokyo2006_cirque_CirqueState state = tokyo2006_cirque_CirqueState_init_zero;
    state.data_mode                  = (tokyo2006_cirque_DataMode)cirque_state_get_data_mode(dev);
    state.sensitivity                = (tokyo2006_cirque_Sensitivity)cirque_state_get_sensitivity(dev);
    state.invert_x                   = cirque_state_get_invert_x(dev);
    state.invert_y                   = cirque_state_get_invert_y(dev);
    state.swap_xy                    = cirque_state_get_swap_xy(dev);
    state.rotate_degrees             = cirque_state_get_rotate_degrees(dev);
    state.primary_tap_enable         = cirque_state_get_primary_tap_enable(dev);
    state.secondary_tap_enable       = cirque_state_get_secondary_tap_enable(dev);
    state.aux_tap_enable             = cirque_state_get_aux_tap_enable(dev);
    state.tap_max_ms                 = cirque_state_get_tap_max_ms(dev);
    state.tap_max_movement           = cirque_state_get_tap_max_movement(dev);
    state.tap_click_ms               = cirque_state_get_tap_click_ms(dev);
    state.tap_drag_enable            = cirque_state_get_tap_drag_enable(dev);
    state.tap_drag_timeout_ms        = cirque_state_get_tap_drag_timeout_ms(dev);
    state.tap_drag_max_movement      = cirque_state_get_tap_drag_max_movement(dev);
    state.secondary_tap_area_width   = cirque_state_get_secondary_tap_area_width(dev);
    state.secondary_tap_area_height  = cirque_state_get_secondary_tap_area_height(dev);
    state.aux_tap_area_width         = cirque_state_get_aux_tap_area_width(dev);
    state.aux_tap_area_height        = cirque_state_get_aux_tap_area_height(dev);
    state.edge_motion_enable         = cirque_state_get_edge_motion_enable(dev);
    state.edge_motion_zone           = cirque_state_get_edge_motion_zone(dev);
    state.edge_motion_speed          = cirque_state_get_edge_motion_speed(dev);
    state.edge_motion_interval_ms    = cirque_state_get_edge_motion_interval_ms(dev);
    state.edge_motion_start_ms       = cirque_state_get_edge_motion_start_ms(dev);
    state.right_edge_scroll_enable   = cirque_state_get_right_edge_scroll_enable(dev);
    state.top_edge_scroll_enable     = cirque_state_get_top_edge_scroll_enable(dev);
    state.scroll_zone                = cirque_state_get_scroll_zone(dev);
    state.scroll_divisor             = cirque_state_get_scroll_divisor(dev);
    state.invert_scroll              = cirque_state_get_invert_scroll(dev);
    state.relative_multiplier        = cirque_state_get_relative_multiplier(dev);
    state.relative_divisor           = cirque_state_get_relative_divisor(dev);
    state.absolute_relative_multiplier = cirque_state_get_abs_relative_multiplier(dev);
    state.absolute_relative_divisor  = cirque_state_get_abs_relative_divisor(dev);
    state.sleep_mode_enable          = cirque_state_get_sleep_mode_enable(dev);
    state.drag_scroll_enabled        = zmk_drag_scroll_is_enabled();
    state.pointer_speed_position     = zmk_pointing_speed_get_position(ZMK_POINTING_SPEED_TARGET_POINTER);
    state.scroll_speed_position      = zmk_pointing_speed_get_position(ZMK_POINTING_SPEED_TARGET_SCROLL);

    /* proto3 `optional` requires explicit presence: mark every field present so
     * nanopb actually serializes it. Without this the response encodes empty. */
    state.has_data_mode = true;
    state.has_sensitivity = true;
    state.has_invert_x = true;
    state.has_invert_y = true;
    state.has_swap_xy = true;
    state.has_rotate_degrees = true;
    state.has_primary_tap_enable = true;
    state.has_secondary_tap_enable = true;
    state.has_aux_tap_enable = true;
    state.has_tap_max_ms = true;
    state.has_tap_max_movement = true;
    state.has_tap_click_ms = true;
    state.has_tap_drag_enable = true;
    state.has_tap_drag_timeout_ms = true;
    state.has_tap_drag_max_movement = true;
    state.has_secondary_tap_area_width = true;
    state.has_secondary_tap_area_height = true;
    state.has_aux_tap_area_width = true;
    state.has_aux_tap_area_height = true;
    state.has_edge_motion_enable = true;
    state.has_edge_motion_zone = true;
    state.has_edge_motion_speed = true;
    state.has_edge_motion_interval_ms = true;
    state.has_edge_motion_start_ms = true;
    state.has_right_edge_scroll_enable = true;
    state.has_top_edge_scroll_enable = true;
    state.has_scroll_zone = true;
    state.has_scroll_divisor = true;
    state.has_invert_scroll = true;
    state.has_relative_multiplier = true;
    state.has_relative_divisor = true;
    state.has_absolute_relative_multiplier = true;
    state.has_absolute_relative_divisor = true;
    state.has_sleep_mode_enable = true;
    state.has_drag_scroll_enabled = true;
    state.has_pointer_speed_position = true;
    state.has_scroll_speed_position = true;

    tokyo2006_cirque_GetStateResponse get_resp = tokyo2006_cirque_GetStateResponse_init_zero;
    get_resp.state = state;
    get_resp.has_state = true;
    resp->which_response_type = tokyo2006_cirque_Response_get_state_tag;
    resp->response_type.get_state = get_resp;
    return 0;
}

static int handle_set_state(const tokyo2006_cirque_SetStateRequest *req,
                            tokyo2006_cirque_Response *resp) {
    /* dev may be NULL -- see handle_get_state. */
    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);

    /* If persist=true, mark so settings subsystem can save later */
    bool persist = req->persist;

    /* Apply each field. We do NOT abort on first error — apply what we can
     * and report the first failure in the response. This matches the
     * template's permissive-update policy.
     */
    int first_rc = 0;
    char first_err[128] = {0};

#define APPLY(field, set_call)                                                  \
    do {                                                                        \
        int rc = set_call;                                                      \
        if (rc != 0 && first_rc == 0) {                                         \
            first_rc = rc;                                                      \
            snprintf(first_err, sizeof(first_err), "%s failed (%d)", #field, rc); \
        }                                                                       \
    } while (0)

    if (req->state.has_data_mode) {
        APPLY(data_mode,            cirque_state_set_data_mode(dev, req->state.data_mode));
    }
    if (req->state.has_sensitivity) {
        APPLY(sensitivity,          cirque_state_set_sensitivity(dev, req->state.sensitivity));
    }
    if (req->state.has_invert_x) {
        APPLY(invert_x,             cirque_state_set_invert_x(dev, req->state.invert_x));
    }
    if (req->state.has_invert_y) {
        APPLY(invert_y,             cirque_state_set_invert_y(dev, req->state.invert_y));
    }
    if (req->state.has_swap_xy) {
        APPLY(swap_xy,              cirque_state_set_swap_xy(dev, req->state.swap_xy));
    }
    if (req->state.has_rotate_degrees) {
        APPLY(rotate_degrees,       cirque_state_set_rotate_degrees(dev, req->state.rotate_degrees));
    }
    if (req->state.has_primary_tap_enable) {
        APPLY(primary_tap_enable,   cirque_state_set_primary_tap_enable(dev, req->state.primary_tap_enable));
    }
    if (req->state.has_secondary_tap_enable) {
        APPLY(secondary_tap_enable, cirque_state_set_secondary_tap_enable(dev, req->state.secondary_tap_enable));
    }
    if (req->state.has_aux_tap_enable) {
        APPLY(aux_tap_enable,       cirque_state_set_aux_tap_enable(dev, req->state.aux_tap_enable));
    }
    if (req->state.has_tap_max_ms) {
        APPLY(tap_max_ms,           cirque_state_set_tap_max_ms(dev, req->state.tap_max_ms));
    }
    if (req->state.has_tap_max_movement) {
        APPLY(tap_max_movement,     cirque_state_set_tap_max_movement(dev, req->state.tap_max_movement));
    }
    if (req->state.has_tap_click_ms) {
        APPLY(tap_click_ms,         cirque_state_set_tap_click_ms(dev, req->state.tap_click_ms));
    }
    if (req->state.has_tap_drag_enable) {
        APPLY(tap_drag_enable,      cirque_state_set_tap_drag_enable(dev, req->state.tap_drag_enable));
    }
    if (req->state.has_tap_drag_timeout_ms) {
        APPLY(tap_drag_timeout_ms,  cirque_state_set_tap_drag_timeout_ms(dev, req->state.tap_drag_timeout_ms));
    }
    if (req->state.has_tap_drag_max_movement) {
        APPLY(tap_drag_max_movement, cirque_state_set_tap_drag_max_movement(dev, req->state.tap_drag_max_movement));
    }
    if (req->state.has_secondary_tap_area_width) {
        APPLY(secondary_tap_area_width, cirque_state_set_secondary_tap_area_width(dev, req->state.secondary_tap_area_width));
    }
    if (req->state.has_secondary_tap_area_height) {
        APPLY(secondary_tap_area_height, cirque_state_set_secondary_tap_area_height(dev, req->state.secondary_tap_area_height));
    }
    if (req->state.has_aux_tap_area_width) {
        APPLY(aux_tap_area_width,   cirque_state_set_aux_tap_area_width(dev, req->state.aux_tap_area_width));
    }
    if (req->state.has_aux_tap_area_height) {
        APPLY(aux_tap_area_height,  cirque_state_set_aux_tap_area_height(dev, req->state.aux_tap_area_height));
    }
    if (req->state.has_edge_motion_enable) {
        APPLY(edge_motion_enable,   cirque_state_set_edge_motion_enable(dev, req->state.edge_motion_enable));
    }
    if (req->state.has_edge_motion_zone) {
        APPLY(edge_motion_zone,     cirque_state_set_edge_motion_zone(dev, req->state.edge_motion_zone));
    }
    if (req->state.has_edge_motion_speed) {
        APPLY(edge_motion_speed,    cirque_state_set_edge_motion_speed(dev, req->state.edge_motion_speed));
    }
    if (req->state.has_edge_motion_interval_ms) {
        APPLY(edge_motion_interval_ms, cirque_state_set_edge_motion_interval_ms(dev, req->state.edge_motion_interval_ms));
    }
    if (req->state.has_edge_motion_start_ms) {
        APPLY(edge_motion_start_ms, cirque_state_set_edge_motion_start_ms(dev, req->state.edge_motion_start_ms));
    }
    if (req->state.has_right_edge_scroll_enable) {
        APPLY(right_edge_scroll_enable, cirque_state_set_right_edge_scroll_enable(dev, req->state.right_edge_scroll_enable));
    }
    if (req->state.has_top_edge_scroll_enable) {
        APPLY(top_edge_scroll_enable, cirque_state_set_top_edge_scroll_enable(dev, req->state.top_edge_scroll_enable));
    }
    if (req->state.has_scroll_zone) {
        APPLY(scroll_zone,          cirque_state_set_scroll_zone(dev, req->state.scroll_zone));
    }
    if (req->state.has_scroll_divisor) {
        APPLY(scroll_divisor,       cirque_state_set_scroll_divisor(dev, req->state.scroll_divisor));
    }
    if (req->state.has_invert_scroll) {
        APPLY(invert_scroll,        cirque_state_set_invert_scroll(dev, req->state.invert_scroll));
    }
    if (req->state.has_relative_multiplier) {
        APPLY(relative_multiplier,  cirque_state_set_relative_multiplier(dev, req->state.relative_multiplier));
    }
    if (req->state.has_relative_divisor) {
        APPLY(relative_divisor,     cirque_state_set_relative_divisor(dev, req->state.relative_divisor));
    }
    if (req->state.has_absolute_relative_multiplier) {
        APPLY(abs_relative_multiplier, cirque_state_set_abs_relative_multiplier(dev, req->state.absolute_relative_multiplier));
    }
    if (req->state.has_absolute_relative_divisor) {
        APPLY(abs_relative_divisor, cirque_state_set_abs_relative_divisor(dev, req->state.absolute_relative_divisor));
    }
    if (req->state.has_sleep_mode_enable) {
        APPLY(sleep_mode_enable,    cirque_state_set_sleep_mode_enable(dev, req->state.sleep_mode_enable));
    }

#undef APPLY

    /* Mirror fields (not part of the 34 cirque runtime settings): apply
     * directly against their runtime APIs. */
    if (req->state.has_drag_scroll_enabled) {
        zmk_drag_scroll_set_enabled(req->state.drag_scroll_enabled);
    }
    if (req->state.has_pointer_speed_position) {
        zmk_pointing_speed_set_position(ZMK_POINTING_SPEED_TARGET_POINTER,
                                        (uint8_t)MIN(req->state.pointer_speed_position, 100));
    }
    if (req->state.has_scroll_speed_position) {
        zmk_pointing_speed_set_position(ZMK_POINTING_SPEED_TARGET_SCROLL,
                                        (uint8_t)MIN(req->state.scroll_speed_position, 100));
    }

    if (first_rc != 0) {
        tokyo2006_cirque_ErrorResponse err = tokyo2006_cirque_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "%s", first_err);
        resp->which_response_type = tokyo2006_cirque_Response_error_tag;
        resp->response_type.error = err;
        return first_rc;
    }

    /* Persist if requested. */
    if (persist) {
        (void)cirque_settings_save_all(dev);
    }

    /* Push the updated runtime state into the ASIC registers. */
    (void)cirque_state_apply_all(dev);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_INPUT_PINNACLE2)
    /* Notify the peripheral that central state changed, shipping the full
     * runtime state so it stays in sync. */
    cirque_relay_send_state(dev);
#endif

    /* Return current state */
    tokyo2006_cirque_SetStateResponse set_resp = tokyo2006_cirque_SetStateResponse_init_zero;
    set_resp.persisted = persist;
    /* Fill set_resp.state with the same logic as handle_get_state. To DRY,
     * we synthesize a GetStateRequest and call handle_get_state directly:
     */
    tokyo2006_cirque_GetStateRequest greq = tokyo2006_cirque_GetStateRequest_init_zero;
    handle_get_state(&greq, resp);
    /* But we want set_resp, not get_state in response. Hack: re-extract. */
    set_resp.state = resp->response_type.get_state.state;
    set_resp.has_state = true;
    resp->which_response_type = tokyo2006_cirque_Response_set_state_tag;
    resp->response_type.set_state = set_resp;
    return 0;
}

static int handle_reset(const tokyo2006_cirque_ResetRequest *req,
                        tokyo2006_cirque_Response *resp) {
    /* dev may be NULL -- see handle_get_state. */
    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);

    cirque_state_load_defaults(dev);

    /* Push the reset (default) state into the ASIC registers. */
    (void)cirque_state_apply_all(dev);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_INPUT_PINNACLE2)
    /* Notify peripheral of reset with the full reset state. */
    cirque_relay_send_state(dev);
#endif

    if (req->factory_defaults) {
        (void)cirque_settings_reset_all(dev);
    }

    tokyo2006_cirque_GetStateRequest greq = tokyo2006_cirque_GetStateRequest_init_zero;
    handle_get_state(&greq, resp);

    tokyo2006_cirque_ResetResponse reset_resp = tokyo2006_cirque_ResetResponse_init_zero;
    reset_resp.state = resp->response_type.get_state.state;
    reset_resp.has_state = true;
    resp->which_response_type = tokyo2006_cirque_Response_reset_tag;
    resp->response_type.reset = reset_resp;
    return 0;
}
