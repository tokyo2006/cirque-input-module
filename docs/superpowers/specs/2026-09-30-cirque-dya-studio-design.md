# Cirque Input Module — DYA Studio Custom RPC Integration

**Status:** draft
**Date:** 2026-09-30
**Branch:** `feat/dya-studio-rpc` (target branch; main branch unaffected)
**Author:** brainstorming session with user (tokyo2006)
**Scope:** Full runtime configurability of Cirque Pinnacle 1CA027 trackpad via DYA Studio

## 1. Summary

Transform `tokyo2006/cirque-input-module` so that every currently devicetree-only
Cirque Pinnacle setting — plus the already-runtime API surface — is exposed as a
**DYA Studio Custom RPC endpoint**, allowing real-time configuration from the
web UI without reflashing.

The work follows the [cormoran ZMK Module Template for Custom Studio RPC][tmpl]
architecture verbatim:

- protobuf message definitions
- nanopb-generated C bindings
- `ZMK_RPC_CUSTOM_SUBSYSTEM` firmware handler
- `zmk-feature-custom-settings` persistence
- split-relay for cross-half state sync
- React + `@cormoran/zmk-studio-react-hook` web UI
- full test pyramid (unit / Renode / BLE / web-e2e)

[tmpl]: https://github.com/cormoran/zmk-module-template-with-custom-studio-rpc

## 2. Goals

1. **All 25+ currently-DT-only Cirque settings** become runtime-mutable without
   rebuild: `data-mode`, `sensitivity`, axis transforms, tap behavior, edge
   motion, edge scroll, pointer scaling, sleep, drag-scroll, pointing speed.
2. **Already-runtime APIs** (`zmk_cirque_mode_*`, `zmk_pointing_speed_*`,
   `zmk_drag_scroll_*`) are wired into the same state struct and exposed via the
   same RPC endpoint.
3. **Persisted across reboot** via `zmk-feature-custom-settings` — the user
   toggles a setting in DYA Studio, walks away, comes back next week, settings
   are still there.
4. **Split-keyboard safe** — central side receives RPC, relays state to
   peripheral(s); settings are applied on both halves.
5. **No regression** for `main` branch users. The DYA Studio work lives on
   `feat/dya-studio-rpc`; main continues to work against `cormoran/zmk#main+dya`.

## 3. Non-goals

- **Not** replacing nxtkb's existing runtime API surface — we extend it.
- **Not** re-architecting the driver — only refactor it to call into a new
  `cirque_state` layer.
- **Not** building a fancy web UI (heatmaps, drag previews). Follow the
  cormoran template's default React form layout.
- **Not** supporting multiple concurrent trackpad instances in one build. The
  driver data model already supports per-device state; the RPC endpoint talks to
  a single device (the one bound to the subsystem). Multi-instance is possible
  later by keying requests by device label.
- **Not** a fork of `petejohanson/cirque-input-module`. We keep the nxtkb
  baseline (which already has the runtime API scaffolding).

## 4. Architectural Decisions

| Decision | Choice | Rationale |
|---|---|---|
| ZMK fork / branch | `cormoran/zmk#main+custom-studio-protocol` | Custom RPC headers (`<zmk/studio/custom.h>`, `ZMK_RPC_CUSTOM_SUBSYSTEM`) live only on this branch — `main+dya` doesn't have them |
| Existing branch | Keep `main` working with `cormoran/zmk#main+dya` (no RPC) | Avoid breaking existing users |
| New branch | `feat/dya-studio-rpc` — full feature | Per the user's choice (option B) |
| Runtime state location | per-device (`struct cirque_runtime_state` in driver data) | Supports future multi-trackpad; cheap to do now |
| Persistence | `zmk-feature-custom-settings` (one setting per field) | Standard pattern in cormoran ecosystem |
| Split relay | Forward the central's full state to peripherals on `set_state` | Matches template's `template_relay.c` |
| Web UI tech | React + TS + Vite + `@cormoran/zmk-studio-react-hook` | Direct copy from template |
| Namespace | `tokyo2006__cirque` (proto package `tokyo2006.cirque`) | Per cormoran template convention `init_module.py --namespace tokyo2006 --module cirque` |
| Test scope | Full pyramid (unit + Renode + BLE + web-e2e) | Per user's choice (option C) |
| Web UI hosting | GitHub Pages from `feat/dya-studio-rpc` | Built-in Actions workflow |

## 5. Repository Layout

The `feat/dya-studio-rpc` branch adds these new trees on top of the existing
`main` tree (existing files modified are flagged with `[M]`):

```
tokyo2006/cirque-input-module/
├── drivers/input/                (M) input_pinnacle.c — calls into cirque_state
│                                   (M) input_pinnacle.h — exposes runtime accessors
│                                       Kconfig        — unchanged
├── src/
│   ├── behaviors/                (existing — left alone for now; revisited if state_tap overlaps)
│   ├── events/                   (existing — trackpad_status_changed reused)
│   ├── studio/                   (NEW) cirque_studio_handler.c    ← RPC
│   │           (NEW) cirque_state.c / cirque_state.h             ← runtime state
│   │           (NEW) cirque_settings.c / cirque_settings.h       ← persistence
│   └── split/                    (NEW) cirque_relay.c / cirque_relay.h
├── include/zmk/                  (existing — left alone)
├── proto/tokyo2006/cirque/       (NEW) cirque.proto               ← nanopb source
├── dts/bindings/                 (existing — minor compat tightening in yaml)
├── web/                          (NEW) React app (template-derived)
│   ├── src/App.tsx, components/, hooks/
│   ├── package.json
│   └── vite.config.ts
├── tests/
│   ├── studio/                   (NEW) RPC handler unit tests
│   ├── zmk-config/               (NEW) build smoke + keymap snippets
│   ├── renode/                   (NEW) E2E over USB CDC
│   ├── ble/                      (NEW) split relay via BabbleSim
│   └── web-e2e/                  (NEW — under web/) browser E2E
├── scripts/                      (NEW) init_module.py — adapts cormoran's
├── snippets/                     (NEW — empty for now; reserved)
├── skills/                       (NEW — optional; ZMK DYA migration skill lives in ~/.config/opencode)
├── boards/                       (NEW — empty; template leaves it empty)
├── west.yml                      (M) — points zmk to main+custom-studio-protocol
├── Kconfig                       (M) — adds ZMK_CIRQUE_STUDIO_RPC + ZMK_CIRQUE_CUSTOM_SETTINGS
├── CMakeLists.txt                (M) — wires nanopb + new src trees
├── .github/workflows/            (M / NEW) — copied from template, namespaced
└── README.md                     (M) — adds "DYA Studio Setup" section
```

## 6. Runtime State Layer (`src/studio/cirque_state.{c,h}`)

### 6.1 State struct

```c
struct cirque_runtime_state {
    /* data mode */
    uint8_t data_mode;           /* CIRQUE_DATA_MODE_ABSOLUTE / _RELATIVE */
    uint8_t sensitivity;         /* CIRQUE_SENSITIVITY_1X / _2X */

    /* axis transforms */
    bool invert_x;
    bool invert_y;
    bool swap_xy;
    uint16_t rotate_degrees;     /* 0 / 90 / 180 / 270 */

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

    /* global state mirror (for studio RPC, not driver-internal) */
    bool drag_scroll_enabled;     /* mirrors zmk_drag_scroll_is_enabled */
    uint8_t pointer_speed_position;
    uint8_t scroll_speed_position;
};
```

### 6.2 Per-device embedding

`struct cirque_pinnacle_data` (existing) gains:

```c
struct cirque_pinnacle_data {
    /* ... existing fields ... */
    struct cirque_runtime_state rt;
};
```

### 6.3 API surface

`include/zmk/cirque_state.h` (new) — installed in include/zmk/:

```c
/* getters — read live state */
uint8_t cirque_state_get_data_mode(const struct device *dev);
uint8_t cirque_state_get_sensitivity(const struct device *dev);
bool    cirque_state_get_invert_x(const struct device *dev);
/* ... one per field ... */

/* setters — validate, write, optionally re-apply to hardware, raise event */
int cirque_state_set_data_mode(const struct device *dev, uint8_t mode);
int cirque_state_set_invert_x(const struct device *dev, bool invert);
/* ... one per field ... */

/* bulk ops */
int cirque_state_load_defaults(const struct device *dev);
int cirque_state_load_from_dt(const struct device *dev);
int cirque_state_load_from_settings(const struct device *dev);  /* called from settings handler */
int cirque_state_apply_all(const struct device *dev);          /* push everything to ASIC */
```

Each `set_*` performs: **range-check → write state → update driver-internal
caches → if relevant, write ASIC register → raise `trackpad_status_changed`**.

### 6.4 Driver refactor

`input_pinnacle.c` is modified **minimally**:

- `DEVICE_DT_INST_DEFINE` macro gets a new `cirque_state_load_from_dt(dev)` call
  in its init function.
- All DT property reads (`DT_PROP(...)`, `DT_INST_PROP(...)`) inside
  per-frame hot path are replaced with `cirque_state_get_*(dev)` calls.
- Mode-switch code paths (`cirque_pinnacle_set_mode_relative`/`_absolute`)
  delegate to `cirque_state_set_data_mode(dev, ...)`.

Hot path performance: getter is a struct field read (one indirection). Negligible
cost vs the existing I²C/SPI transaction.

## 7. Protobuf Definitions

### 7.1 Message schema (`proto/tokyo2006/cirque/cirque.proto`)

```proto
syntax = "proto3";
package tokyo2006.cirque;

enum DataMode {
    DATA_MODE_ABSOLUTE = 0;
    DATA_MODE_RELATIVE = 1;
}

enum Sensitivity {
    SENSITIVITY_1X = 0;
    SENSITIVITY_2X = 1;
}

message CirqueState {
    DataMode data_mode = 1;
    Sensitivity sensitivity = 2;

    bool invert_x = 3;
    bool invert_y = 4;
    bool swap_xy = 5;
    uint32 rotate_degrees = 6;

    bool primary_tap_enable = 10;
    bool secondary_tap_enable = 11;
    bool aux_tap_enable = 12;
    uint32 tap_max_ms = 13;
    uint32 tap_max_movement = 14;
    uint32 tap_click_ms = 15;
    bool tap_drag_enable = 16;
    uint32 tap_drag_timeout_ms = 17;
    uint32 tap_drag_max_movement = 18;
    uint32 secondary_tap_area_width = 19;
    uint32 secondary_tap_area_height = 20;
    uint32 aux_tap_area_width = 21;
    uint32 aux_tap_area_height = 22;

    bool edge_motion_enable = 30;
    uint32 edge_motion_zone = 31;
    uint32 edge_motion_speed = 32;
    uint32 edge_motion_interval_ms = 33;
    uint32 edge_motion_start_ms = 34;

    bool right_edge_scroll_enable = 40;
    bool top_edge_scroll_enable = 41;
    uint32 scroll_zone = 42;
    uint32 scroll_divisor = 43;
    bool invert_scroll = 44;

    uint32 relative_multiplier = 50;
    uint32 relative_divisor = 51;
    uint32 absolute_relative_multiplier = 52;
    uint32 absolute_relative_divisor = 53;

    bool sleep_mode_enable = 60;
    bool drag_scroll_enabled = 80;
    uint32 pointer_speed_position = 90;
    uint32 scroll_speed_position = 91;
}

message GetStateRequest {}
message GetStateResponse { CirqueState state = 1; }

message SetStateRequest {
    CirqueState state = 1;
    bool persist = 2;
}

message SetStateResponse {
    CirqueState state = 1;
    bool persisted = 2;
}

message ResetRequest {
    bool factory_defaults = 1;
}
message ResetResponse { CirqueState state = 1; }

message Request {
    oneof request_type {
        GetStateRequest get_state = 1;
        SetStateRequest set_state = 2;
        ResetRequest reset = 3;
    }
}

message ErrorResponse { string message = 1; }

message Response {
    oneof response_type {
        ErrorResponse error = 1;
        GetStateResponse get_state = 2;
        SetStateResponse set_state = 3;
        ResetResponse reset = 4;
    }
}
```

### 7.2 Field-mapping policy

- Single `CirqueState` message containing every field (no `GetTapConfigRequest`
  / `SetTapConfigRequest` etc.). Rationale: simpler; future per-section growth
  handled by adding new fields, not new messages. If field count exceeds ~60,
  split then.
- `rotate_degrees` is `uint32` even though values are 0/90/180/270 (not a
  bitmask) for forward compatibility with future non-90° rotations.
- All `uint16`/`uint32` width choices are generous; C-side range validation
  enforces actual bounds.
- `state_version` field is **not** added in this spec. If a setting-schema
  version is later needed (e.g. for migration when defaults change), add at
  field 100 then.

## 8. Firmware RPC Handler (`src/studio/cirque_studio_handler.c`)

Follows the cormoran template's `template_handler.c` structure 1:1.

### 8.1 Subsystem registration

```c
static struct zmk_rpc_custom_subsystem_meta cirque_feature_meta = {
    ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS(
        "https://tokyo2006.github.io/cirque-input-module/"),
    .security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

ZMK_RPC_CUSTOM_SUBSYSTEM(tokyo2006__cirque, &cirque_feature_meta,
                         cirque_rpc_handle_request);

ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(tokyo2006__cirque,
                                         tokyo2006_cirque_Response);
```

### 8.2 Request dispatch

```c
static bool cirque_rpc_handle_request(const zmk_custom_CallRequest *raw,
                                       pb_callback_t *encode_response) {
    /* decode → switch on req.which_request_type → handle_* → fill resp */
    /* on unknown: ErrorResponse */
}
```

### 8.3 Handlers

- `handle_get_state` — gather all `cirque_state_get_*()` calls into a
  `CirqueState`, return.
- `handle_set_state` — apply each field via `cirque_state_set_*()`, with
  range-check. If `persist` is true, schedule `cirque_settings_save()`. Return
  applied state.
- `handle_reset` — `cirque_state_load_defaults(dev)` + persist (when
  `factory_defaults=true`).

### 8.4 Error policy

- Range violation → `ErrorResponse` with `"<field>: value X out of [min,max]"`.
- Decoding error → generic `"Failed to decode request"`.
- Handler error → generic `"Failed to process request"`.
- Handler always returns `true` (cormoran template convention).

## 9. Persistence (`src/studio/cirque_settings.c`)

Uses `zmk-feature-custom-settings` (cormoran fork). One
`ZMK_CUSTOM_SETTING_DEFINE` per field (≈ 30 entries). Settings are public
(visible to other modules) but only the cirque subsystem can write them
unsecured.

```c
ZMK_CUSTOM_SETTING_DEFINE(cirque_data_mode,
    "tokyo2006__cirque", "data_mode",
    ZMK_CUSTOM_SETTING_VALUE_TYPE_U8,
    ZMK_CUSTOM_SETTING_VALUE_U8(CIRQUE_DATA_MODE_ABSOLUTE),
    ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
    ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
    ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
    ZMK_CUSTOM_SETTING_NO_CONSTRAINT);
```

### 9.1 Load order

1. `device_init` → `cirque_state_load_defaults(dev)` (hardcoded defaults)
2. `cirque_state_load_from_dt(dev)` (DT values, **if** they differ from
   defaults)
3. `settings_load` → `cirque_settings_load` callback → overwrite state with
   stored values
4. `cirque_state_apply_all(dev)` — push everything to ASIC

This way: **DT < defaults** (no), DT only overrides defaults at first boot.
After the user touches a setting in DYA Studio and persists, settings takes
precedence. Reverting to DT requires `ResetRequest{ factory_defaults=true }`.

## 10. Split Relay (`src/split/cirque_relay.{c,h}`)

### 10.1 Direction

Central-only: `ZMK_RPC_CUSTOM_SUBSYSTEM` lives on central; RPC arrives on
central. Central applies locally + forwards the full `CirqueState` to
peripheral(s) via `zmk_split_peripheral_report_event`.

### 10.2 Wire format

```c
struct cirque_relay_event {
    struct zmk_split_peripheral_event_header header;
    struct tokyo2006_cirque_CirqueState state;
};
```

### 10.3 Peripheral-side apply

Peripheral receives the relay event → calls `cirque_state_apply_all(dev)` →
no re-persistence (central already saved). No RPC handler registered on
peripheral.

### 10.4 Kconfig gate

`cirque_relay.c` is compiled **only when** `CONFIG_ZMK_SPLIT_RELAY_EVENT=y`.
This is the template's pattern.

## 11. Web UI (`web/`)

### 11.1 Tech

- React 18 + TypeScript + Vite (template default)
- `@cormoran/zmk-studio-react-hook` for WebSerial / WebBluetooth transport
- Component library: shadcn/ui (template default) or MUI — defer to template's
  current choice at copy time

### 11.2 Layout

Single page with 8 collapsible `<SectionCard>` sections:

1. **Mode & Sensitivity** — radio (absolute/relative) + radio (1x/2x)
2. **Axis** — 3 switches (invert-x, invert-y, swap-xy) + number stepper (0/90/180/270)
3. **Tap** — 3 enable switches + 5 sliders (timing/movement thresholds)
4. **Edge Motion** — 1 enable + 4 sliders
5. **Edge Scroll** — 2 enables + 2 sliders + invert-scroll switch
6. **Pointer** — 4 sliders (multiplier/divisor pairs)
7. **Speed** — 2 sliders (pointer position / scroll position)
8. **Misc** — sleep-mode enable, drag-scroll enable

Right pane: live state read-out (mirrors server state) with `Refresh` button.

Top: `Connect` button (from `useStudio()`), save status indicator, `Reset to defaults` button.

### 11.3 State management

`useCirqueState()` hook wraps:

- `getState()` on mount + after every `setState`
- `setState(field, value, { persist })` for each input
- Optimistic UI updates with rollback on RPC error
- Debounced drag for sliders (250 ms)

### 11.4 Build & deploy

- Vite `npm run build` → static files in `web/dist/`
- GitHub Actions `.github/workflows/github-pages.yml` deploys on push to
  `feat/dya-studio-rpc` → `gh-pages` branch
- `cormoran.github.io/zmk-module-template/` style preview per PR via
  `pr-previews.yml` (optional, no secrets needed for skip)

## 12. Tests

### 12.1 Unit (`tests/studio/`)

Twister tests + native_posix. Verify:

- `cirque_state_set_*()` range checks (out-of-range → error)
- Default loading matches hardcoded defaults
- `handle_get_state` round-trip — request → response matches in-memory state
- `handle_set_state` — apply + persist + return updated state
- `handle_reset` — factory reset clears settings

### 12.2 Build smoke (`tests/zmk-config/`)

`build.yaml` with `CONFIG_ZMK_CIRQUE_STUDIO_RPC=y`,
`CONFIG_ZMK_CUSTOM_SETTINGS=y`, `CONFIG_ZMK_CUSTOM_SETTINGS_STUDIO_RPC=y`. Compiles
clean.

### 12.3 Renode (`tests/renode/`)

`tests/renode/renode_test.py` boots:

- `usb_wired_central` artifact
- (optional) `usb_wired_peripheral` artifact for split-relay scenario

Steps:

1. Open USB CDC
2. Send `GetStateRequest` via Studio RPC framing
3. Assert response `data_mode == ABSOLUTE`
4. Send `SetStateRequest { state: { data_mode: RELATIVE } }`
5. Send `GetStateRequest`
6. Assert response reflects change
7. Reboot simulated device
8. Assert state persisted

### 12.4 BLE BabbleSim (`tests/ble/`)

`tests/ble/case.yaml` (or equivalent) boots:

- `usb_wired_central` + `usb_wired_peripheral`
- Connect via Studio RPC (BLE GATT)
- Send `SetStateRequest`
- Assert peripheral log line: "Applied relayed state: data_mode=RELATIVE"

### 12.5 Web E2E (`web/e2e/`)

Playwright + Renode. `rpc.spec.ts`:

1. Boot `web_e2e` artifact (Studio locking off — emulator can't press
   `&studio_unlock`)
2. Launch browser, navigate to web UI (locally served)
3. Click `Connect` → chooses Renode's CDC port
4. Click Mode toggle to RELATIVE
5. Assert UI shows RELATIVE after round-trip
6. Reload page
7. Click `Connect` again
8. Assert UI still shows RELATIVE (persistence)

### 12.6 Web unit (`web/src/**.test.tsx`)

Vitest. Covers `useCirqueState()` hook logic + reducer.

## 13. CI/CD (`.github/workflows/`)

Replicates cormoran template's seven workflows:

| Workflow | Trigger | Purpose |
|---|---|---|
| `build.yml` | push, PR | Twister build matrix |
| `unit-test.yml` | push, PR | Native unit tests |
| `web-test.yml` | push, PR | vitest + tsc --noEmit |
| `ble-test.yml` | push, PR | BabbleSim (x86 Linux only) |
| `web-e2e.yml` | push, PR | Playwright + Renode |
| `pr-previews.yml` | PR | Cloudflare Workers preview (optional, skips without secrets) |
| `github-pages.yml` | push to `feat/dya-studio-rpc` | Deploy web UI to `gh-pages` |

Workflow names follow cormoran template; only artifact names change
(`cirque_input_module_*`).

## 14. Migration Path

### 14.1 `main` branch users

- No change. Module behavior is identical.

### 14.2 `feat/dya-studio-rpc` users

1. **West:**

   ```yaml
   projects:
     - name: zmk
       remote: cormoran
       revision: main+custom-studio-protocol   # NOT main+dya
       import: app/west.yml
     - name: cirque-input-module
       remote: tokyo2006
       revision: feat/dya-studio-rpc
   ```

2. **Keyboard `.conf`:**

   ```conf
   CONFIG_ZMK_CIRQUE_STUDIO_RPC=y
   CONFIG_ZMK_CUSTOM_SETTINGS=y
   CONFIG_ZMK_CUSTOM_SETTINGS_STUDIO_RPC=y
   ```

3. **Build:** `west build` as normal; flash.
4. **Use:** Open https://studio.dya.cormoran.works/ → press `&studio_unlock` →
   the **Cirque** tab appears with live state editor.

### 14.3 Caveat (must document prominently)

`main+custom-studio-protocol` and `main+dya` are **mutually exclusive** ZMK
branches. Switching between them requires a clean rebuild. Document this in
README.

## 15. Work Breakdown

| Phase | Estimate | Owner |
|---|---|---|
| `cirque_state` runtime layer + driver refactor | 2–3 days | — |
| Proto + nanopb + handler | 1–2 days | — |
| `zmk-feature-custom-settings` integration | 1 day | — |
| Split relay | 0.5 day | — |
| Web UI (React + hooks) | 3–4 days | — |
| Tests (unit + Renode + BLE + web-e2e) | 2–3 days | — |
| CI workflow + init_module.py adaptation | 1 day | — |
| README + docs | 0.5 day | — |
| **Total** | **~10–15 days** | — |

## 16. Risks

| Risk | Likelihood | Mitigation |
|---|---|---|
| `main+custom-studio-protocol` branch rebases break our module | medium | Pin to a specific SHA, not `main+custom-studio-protocol` rolling |
| Settings schema evolution breaks older firmware | low | Add `state_version` field when needed |
| Peripheral-side runtime state diverges from central after split reconnect | low | Re-broadcast full state on `zmk_split_peripheral_status_changed` |
| Web UI bundle too large for GH Pages soft-limit (1 GB) | low | Use Vite code-splitting; track size in CI |
| nanopb generator version mismatch | medium | Pin nanopb generator commit in `west.yml` |
| Range-check logic diverges between C handler and web UI | low | Single source of truth: C handler defines limits, web UI fetches them via a `GetLimitsRequest` (added if discrepancies appear) |

## 17. Open Questions (defer until implementation)

- Should `handle_reset` also reset `pointer_speed_position`? Current plan: yes
  (matches factory reset semantics).
- Should `handle_set_state` validate that `rotate_degrees ∈ {0, 90, 180, 270}`?
  Current plan: yes; reject 1, 2, etc. with `ErrorResponse`.
- Should we expose `data_ready_gpio` status? No — not user-tunable.
- Web UI: dark/light mode auto? Defer to template default.

## 18. References

- DYA Studio developer guide: https://studio.dya.cormoran.works/developer-guide
- Cormoran module template: https://github.com/cormoran/zmk-module-template-with-custom-studio-rpc
- Cormoran ZMK fork (custom RPC branch): https://github.com/cormoran/zmk/tree/main+custom-studio-protocol
- zmk-feature-custom-settings: https://github.com/cormoran/zmk-feature-custom-settings
- Existing runtime API in nxtkb fork: `include/zmk/cirque_mode.h`,
  `include/zmk/pointing_speed.h`, `include/zmk/drag_scroll.h`
- Cirque Pinnacle datasheet (referenced in `docs/pinnacle-data-output.md`)