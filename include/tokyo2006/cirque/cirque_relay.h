/*
 * SPDX-License-Identifier: MIT
 *
 * Split-relay helper for cirque runtime state sync.
 *
 * When CONFIG_ZMK_SPLIT_RELAY_EVENT is enabled the central uses
 * cirque_relay_send_sample() to ship a tiny version-stamped carrier to the
 * peripheral after every state-changing Studio RPC. The peripheral's handler
 * logs one line confirming receipt.
 *
 * NOTE: the current carrier is a STUB. It only carries a `version` byte and
 * a 32-bit scratch value (typically 0 / a placeholder). It is intentionally
 * NOT a full nanopb-encoded CirqueState. Full state sync (Option A) requires:
 *   (a) moving src/studio/cirque_state.c out of the CONFIG_ZMK_CIRQUE_STUDIO_RPC
 *       gating block in CMakeLists.txt so peripheral halves have the
 *       cirque_state_set_*() setters, and
 *   (b) either bumping CONFIG_ZMK_SPLIT_RELAY_EVENT_DATA_LEN past 128 bytes
 *       (default), or relaying only the runtime-impacting subset of fields.
 * See Task 14 report for the trade-off discussion.
 *
 * The relay carrier is a plain, packed C struct -- NOT protobuf. ZMK's relay
 * (`ZMK_RELAY_EVENT_*`, <zmk/event_manager.h>) memcpy()s the whole event
 * struct into the relay payload byte-for-byte, so both split halves must be
 * built from the same struct definition (same layout). Both halves also run
 * the same CPU architecture (nRF52 / bsim x86, both little-endian), so no
 * byte-swapping is needed; if you ever relay between different-endian halves,
 * serialize fields explicitly instead of memcpy'ing a struct.
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

/* Bump when the wire layout of struct cirque_relay_sample changes. */
#define CIRQUE_RELAY_SAMPLE_VERSION 1

/*
 * The relay carrier. `source` is required by the relay macros' loop guard
 * (ZMK_RELAY_EVENT_SOURCE_SELF on send; stamped to the sender index + 1 on
 * receive). The struct must fit CONFIG_ZMK_SPLIT_RELAY_EVENT_DATA_LEN (ZMK's
 * default 128 is plenty for these few bytes) and the identifier string below
 * must fit CONFIG_ZMK_SPLIT_RELAY_EVENT_TYPE_NAME_LEN (default 4); the relay
 * macros BUILD_ASSERT both.
 */
struct cirque_relay_sample {
    uint8_t source;
    uint8_t version;
    int32_t value;
} __packed;

ZMK_EVENT_DECLARE(cirque_relay_sample);

/*
 * Central-side entry point. Called from the Studio RPC handler after applying
 * state in handle_set_state() and handle_reset(). Raises the relay carrier so
 * the direction macro ships it to the peripheral(s). Only compiled (like this
 * whole helper) when ZMK's CONFIG_ZMK_SPLIT_RELAY_EVENT is enabled -- the
 * helper has no Kconfig of its own.
 *
 * STUB BEHAVIOUR: `value` is currently ignored by the relay wire; it is
 * reserved for the future full-state payload. Central callers may pass 0.
 */
void cirque_relay_send_sample(int32_t value);