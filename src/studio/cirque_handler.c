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

    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);
    if (dev == NULL) {
        tokyo2006_cirque_ErrorResponse err = tokyo2006_cirque_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message),
                 "No cirque,pinnacle2 device found in devicetree");
        resp->which_response_type = tokyo2006_cirque_Response_error_tag;
        resp->response_type.error = err;
        return -ENODEV;
    }

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

    tokyo2006_cirque_GetStateResponse get_resp = tokyo2006_cirque_GetStateResponse_init_zero;
    get_resp.state = state;
    resp->which_response_type = tokyo2006_cirque_Response_get_state_tag;
    resp->response_type.get_state = get_resp;
    return 0;
}

static int handle_set_state(const tokyo2006_cirque_SetStateRequest *req,
                            tokyo2006_cirque_Response *resp) {
    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);
    if (dev == NULL) {
        tokyo2006_cirque_ErrorResponse err = tokyo2006_cirque_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "No cirque,pinnacle2 device");
        resp->which_response_type = tokyo2006_cirque_Response_error_tag;
        resp->response_type.error = err;
        return -ENODEV;
    }

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

    APPLY(data_mode,                cirque_state_set_data_mode(dev, req->state.data_mode));
    APPLY(sensitivity,              cirque_state_set_sensitivity(dev, req->state.sensitivity));
    APPLY(invert_x,                 cirque_state_set_invert_x(dev, req->state.invert_x));
    APPLY(invert_y,                 cirque_state_set_invert_y(dev, req->state.invert_y));
    APPLY(swap_xy,                  cirque_state_set_swap_xy(dev, req->state.swap_xy));
    APPLY(rotate_degrees,           cirque_state_set_rotate_degrees(dev, req->state.rotate_degrees));
    APPLY(primary_tap_enable,       cirque_state_set_primary_tap_enable(dev, req->state.primary_tap_enable));
    APPLY(secondary_tap_enable,     cirque_state_set_secondary_tap_enable(dev, req->state.secondary_tap_enable));
    APPLY(aux_tap_enable,           cirque_state_set_aux_tap_enable(dev, req->state.aux_tap_enable));
    APPLY(tap_max_ms,               cirque_state_set_tap_max_ms(dev, req->state.tap_max_ms));
    APPLY(tap_max_movement,         cirque_state_set_tap_max_movement(dev, req->state.tap_max_movement));
    APPLY(tap_click_ms,             cirque_state_set_tap_click_ms(dev, req->state.tap_click_ms));
    APPLY(tap_drag_enable,          cirque_state_set_tap_drag_enable(dev, req->state.tap_drag_enable));
    APPLY(tap_drag_timeout_ms,      cirque_state_set_tap_drag_timeout_ms(dev, req->state.tap_drag_timeout_ms));
    APPLY(tap_drag_max_movement,    cirque_state_set_tap_drag_max_movement(dev, req->state.tap_drag_max_movement));
    APPLY(secondary_tap_area_width, cirque_state_set_secondary_tap_area_width(dev, req->state.secondary_tap_area_width));
    APPLY(secondary_tap_area_height,cirque_state_set_secondary_tap_area_height(dev, req->state.secondary_tap_area_height));
    APPLY(aux_tap_area_width,       cirque_state_set_aux_tap_area_width(dev, req->state.aux_tap_area_width));
    APPLY(aux_tap_area_height,      cirque_state_set_aux_tap_area_height(dev, req->state.aux_tap_area_height));
    APPLY(edge_motion_enable,       cirque_state_set_edge_motion_enable(dev, req->state.edge_motion_enable));
    APPLY(edge_motion_zone,         cirque_state_set_edge_motion_zone(dev, req->state.edge_motion_zone));
    APPLY(edge_motion_speed,        cirque_state_set_edge_motion_speed(dev, req->state.edge_motion_speed));
    APPLY(edge_motion_interval_ms,  cirque_state_set_edge_motion_interval_ms(dev, req->state.edge_motion_interval_ms));
    APPLY(edge_motion_start_ms,     cirque_state_set_edge_motion_start_ms(dev, req->state.edge_motion_start_ms));
    APPLY(right_edge_scroll_enable, cirque_state_set_right_edge_scroll_enable(dev, req->state.right_edge_scroll_enable));
    APPLY(top_edge_scroll_enable,   cirque_state_set_top_edge_scroll_enable(dev, req->state.top_edge_scroll_enable));
    APPLY(scroll_zone,              cirque_state_set_scroll_zone(dev, req->state.scroll_zone));
    APPLY(scroll_divisor,           cirque_state_set_scroll_divisor(dev, req->state.scroll_divisor));
    APPLY(invert_scroll,            cirque_state_set_invert_scroll(dev, req->state.invert_scroll));
    APPLY(relative_multiplier,      cirque_state_set_relative_multiplier(dev, req->state.relative_multiplier));
    APPLY(relative_divisor,         cirque_state_set_relative_divisor(dev, req->state.relative_divisor));
    APPLY(abs_relative_multiplier,  cirque_state_set_abs_relative_multiplier(dev, req->state.absolute_relative_multiplier));
    APPLY(abs_relative_divisor,     cirque_state_set_abs_relative_divisor(dev, req->state.absolute_relative_divisor));
    APPLY(sleep_mode_enable,        cirque_state_set_sleep_mode_enable(dev, req->state.sleep_mode_enable));

#undef APPLY

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

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
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
    resp->which_response_type = tokyo2006_cirque_Response_set_state_tag;
    resp->response_type.set_state = set_resp;
    return 0;
}

static int handle_reset(const tokyo2006_cirque_ResetRequest *req,
                        tokyo2006_cirque_Response *resp) {
    const struct device *dev = DEVICE_DT_GET_ANY(cirque_pinnacle2);
    if (dev == NULL) {
        tokyo2006_cirque_ErrorResponse err = tokyo2006_cirque_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "No cirque,pinnacle2 device");
        resp->which_response_type = tokyo2006_cirque_Response_error_tag;
        resp->response_type.error = err;
        return -ENODEV;
    }

    cirque_state_load_defaults(dev);

    /* Push the reset (default) state into the ASIC registers. */
    (void)cirque_state_apply_all(dev);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
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
    resp->which_response_type = tokyo2006_cirque_Response_reset_tag;
    resp->response_type.reset = reset_resp;
    return 0;
}
