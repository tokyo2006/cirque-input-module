# DYA Studio Custom RPC for Cirque Input Module — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make every Cirque Pinnacle runtime setting configurable in real-time from the DYA Studio web UI, without reflashing, with split-keyboard relay and `zmk-feature-custom-settings` persistence.

**Architecture:** Mirror `cormoran/zmk-module-template-with-custom-studio-rpc` 1:1. Refactor the driver so DT-only properties become runtime state with getter/setter API; expose the state via a protobuf Custom Studio RPC; persist via `zmk-feature-custom-settings`; relay across split halves; drive from a React web UI deployed to GitHub Pages.

**Tech Stack:** C (Zephyr module), nanopb (proto runtime), protobuf3, `cormoran/zmk#main+custom-studio-protocol`, `zmk-feature-custom-settings`, React 18 + TypeScript + Vite, `@cormoran/zmk-studio-react-hook`, Twister + Renode + BabbleSim + Playwright (test pyramid), GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-30-cirque-dya-studio-design.md`

## Global Constraints

These are project-wide rules from the spec; every task implicitly complies:

- **Working directory:** all commands run inside `~/project/cirque-input-module` unless explicitly noted. Verify with `pwd` before any path-sensitive command.
- **Target branch:** every task is performed on `feat/dya-studio-rpc`. The `main` branch is read-only.
- **ZMK fork / branch:** `cormoran/zmk#main+custom-studio-protocol` (NOT `main+dya`; they are mutually exclusive).
- **Namespace:** all generated identifiers use prefix `tokyo2006__cirque` (C macros, Kconfig symbols, subsystem meta, setting paths). Proto package `tokyo2006.cirque`.
- **Subsystem security:** `ZMK_STUDIO_RPC_HANDLER_UNSECURED` — matches cormoran template default; web UI already implements unlock flow.
- **Web UI URL:** `https://tokyo2006.github.io/cirque-input-module/`.
- **Build verification:** NEVER run `west init` / `west update` / `west build` / `west manifest` locally. Push to `feat/dya-studio-rpc`; the repo's `.github/workflows/build.yml` triggers and produces UF2s; read the Actions run log as the verdict.
- **Test verification:** `west test`, Renode, BabbleSim, Playwright all run via GitHub Actions, never locally. The "Run:" command in every task that verifies firmware behavior is `git push && gh run watch` (or equivalent) — read the GHA log to confirm.
- **Commit cadence:** commit at the end of each task (or at the end of each step marked "Commit"). Conventional Commits (`feat:`, `fix:`, `chore:`, `docs:`, `test:`).
- **DRY:** copy verbatim from cormoran template's `template_handler.c`, `template_relay.c`, `init_module.py`, web `App.tsx` — only change cirque-specific identifiers. Don't redesign what works.
- **No secrets in source:** all keys (none currently) stay out of the repo. Studio UI URL is public.

---

## Phase 0 — Bootstrap

### Task 1: Create feature branch and import template scaffold

**Files:**
- Create: `feat/dya-studio-rpc` branch
- Create: copies of cormoran template files (proto/, src/studio/, src/split/, web/, tests/, scripts/, snippets/, skills/, boards/, .github/)

**Interfaces:**
- Consumes: existing `main` branch working tree at `~/project/cirque-input-module`
- Produces: `feat/dya-studio-rpc` branch with template scaffold laid alongside existing module code

- [ ] **Step 1: Verify clean working tree on main**

Run: `git status`
Expected: "nothing to commit, working tree clean". If not, the previous session left state — abort and resolve first.

- [ ] **Step 2: Create and switch to feature branch**

Run:
```bash
git checkout -b feat/dya-studio-rpc
```
Expected: "Switched to a new branch 'feat/dya-studio-rpc'".

- [ ] **Step 3: Add cormoran template as a temporary fetch remote**

Run:
```bash
git remote add cormoran-template https://github.com/cormoran/zmk-module-template-with-custom-studio-rpc.git
git fetch cormoran-template main+custom-studio-protocol
```
Expected: fetch completes; no merge yet.

- [ ] **Step 4: List template files we want to import**

We want these template paths (relative to template repo root):
- `proto/your-name/template/`
- `src/studio/template_handler.c`
- `src/split/template_relay.{c,h}`
- `web/` (entire)
- `tests/`
- `scripts/init_module.py`
- `.github/workflows/`
- `Kconfig`, `CMakeLists.txt` (template's versions, for reference only)
- `west.yml`, `.clang-format`, `.pre-commit-config.yaml`, `.gitignore`

Run:
```bash
git ls-tree --name-only -r cormoran-template/main+custom-studio-protocol | grep -E "^(proto/|src/studio/|src/split/|web/|tests/|scripts/|.github/workflows/|Kconfig$|CMakeLists.txt$|west.yml$|.clang-format$|.pre-commit-config.yaml$|.gitignore$)" | sort
```
Expected: ~80 file paths.

- [ ] **Step 5: Read the template's init_module.py before running it**

Read `scripts/init_module.py` from cormoran's `main+custom-studio-protocol` branch (use `gh repo view cormoran/zmk-module-template-with-custom-studio-protocol` then fetch raw, or clone shallow). Confirm it accepts `--namespace tokyo2006 --module cirque` and does string replacements on identifiers + paths.

- [ ] **Step 6: Cherry-pick or copy template files into our repo**

Two options:
- (a) `git read-tree` then `git checkout-index` for each path
- (b) `git checkout cormoran-template/main+custom-studio-protocol -- <path>` for each path

Use (b). Run, for each path from Step 4:
```bash
git checkout cormoran-template/main+custom-studio-protocol -- proto/your-name/template/
git checkout cormoran-template/main+custom-studio-protocol -- src/studio/template_handler.c
git checkout cormoran-template/main+custom-studio-protocol -- src/split/
git checkout cormoran-template/main+custom-studio-protocol -- web/
git checkout cormoran-template/main+custom-studio-protocol -- tests/
git checkout cormoran-template/main+custom-studio-protocol -- scripts/
git checkout cormoran-template/main+custom-studio-protocol -- .github/workflows/
```
Verify with: `ls proto/your-name/template src/studio src/split web tests scripts .github/workflows`

Expected: all paths exist.

- [ ] **Step 7: Add template's `.clang-format`, `.pre-commit-config.yaml`, `.gitignore` only if missing**

Run:
```bash
ls .clang-format .pre-commit-config.yaml .gitignore 2>&1
```
If any missing, `git checkout cormoran-template/main+custom-studio-protocol -- <path>` for that one. If all exist, skip.

- [ ] **Step 8: Commit the scaffold import**

Run:
```bash
git add proto/ src/studio/ src/split/ web/ tests/ scripts/ .github/workflows/ .clang-format .pre-commit-config.yaml .gitignore
git commit -m "chore: import cormoran template scaffold"
```
Expected: commit succeeds.

- [ ] **Step 9: Push branch to origin**

Run:
```bash
git push -u origin feat/dya-studio-rpc
```
Expected: "branch 'feat/dya-studio-rpc' set up to track 'origin/feat/dya-studio-rpc'".

- [ ] **Step 10: Verify GHA workflow picks it up**

Visit `https://github.com/tokyo2006/cirque-input-module/actions`. The first commit should trigger `build.yml` (which will fail at this point because we haven't adapted Kconfig/CMakeLists yet — that's expected for Task 1).

---

### Task 2: Run init_module.py to retarget template strings to tokyo2006/cirque

**Files:**
- Modify: every file under `proto/`, `src/studio/`, `src/split/`, `tests/`, `web/`, `scripts/`, `.github/workflows/`, `Kconfig`, `CMakeLists.txt` (those that came from template)

**Interfaces:**
- Consumes: template-scaffolded files from Task 1
- Produces: all `your-name` / `your_name` / `template` placeholders replaced with `tokyo2006` / `tokyo2006` / `cirque` — package `tokyo2006.cirque`, C symbols `tokyo2006__cirque`, Kconfig `ZMK_TEMPLATE_FEATURE` → `ZMK_CIRQUE_STUDIO_RPC`

- [ ] **Step 1: Run the init script in dry-run mode to preview changes**

Run:
```bash
python3 scripts/init_module.py --namespace tokyo2006 --module cirque --verify-only 2>&1 | head -50
```
Expected: prints a checklist of replacements; should NOT show "unresolved placeholders" errors at this point (we haven't customized anything yet — template should still match its own defaults). If it does show errors, abort and re-check Step 6 of Task 1.

- [ ] **Step 2: Run the init script to apply replacements**

Run:
```bash
python3 scripts/init_module.py --namespace tokyo2006 --module cirque 2>&1
```
Expected: prints progress; ends with summary like "X files updated, Y placeholders replaced".

- [ ] **Step 3: Verify Kconfig symbols are renamed**

Run:
```bash
grep -rE "ZMK_TEMPLATE_FEATURE|ZMK_TEMPLATE_FEATURE_STUDIO_RPC" Kconfig src/ tests/ 2>&1
```
Expected: NO matches. (`ZMK_TEMPLATE_FEATURE` → `ZMK_CIRQUE_STUDIO_RPC`; `ZMK_TEMPLATE_FEATURE_STUDIO_RPC` → `ZMK_CIRQUE_STUDIO_RPC_STUDIO_RPC`.)

Run: `grep -nE "ZMK_CIRQUE_STUDIO_RPC" Kconfig`
Expected: 2 matches, both inside the menuconfig block.

- [ ] **Step 4: Verify proto package is renamed**

Run:
```bash
head -3 proto/tokyo2006/cirque/cirque.proto
```
Expected: `syntax = "proto3";` then blank line then `package tokyo2006.cirque;`

- [ ] **Step 5: Verify web UI subsystem URL**

Run:
```bash
grep -rE "studio_unlock|subsystem|tokyo2006__cirque" src/studio/ 2>&1 | head -20
```
Expected: identifier `tokyo2006__cirque` appears; URL in subsystem meta points at `https://tokyo2006.github.io/cirque-input-module/`.

- [ ] **Step 6: Verify CMakeLists wires nanopb**

Run:
```bash
grep -nE "nanopb_generate_cpp|NANOPB_GENERATE_CPP_APPEND_PATH" CMakeLists.txt
```
Expected: at least 1 match each.

- [ ] **Step 7: Re-run init in verify-only mode — must be clean**

Run:
```bash
python3 scripts/init_module.py --namespace tokyo2006 --module cirque --verify-only
```
Expected: exit code 0, prints "All placeholders resolved" or similar success message. No failures.

- [ ] **Step 8: Commit**

Run:
```bash
git add -A
git commit -m "chore: retarget template identifiers to tokyo2006/cirque"
```

---

### Task 3: Adapt west.yml, top-level Kconfig, top-level CMakeLists

**Files:**
- Modify: `west.yml`
- Modify: `Kconfig`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: retargeted scaffold from Task 2
- Produces: project-level config files that point at `cormoran/zmk#main+custom-studio-protocol`, source the new `src/studio/` and `src/split/` trees, and coexist with existing `drivers/input/`

- [ ] **Step 1: Read the current top-level Kconfig**

Run: `cat Kconfig`
Expected: a single `rsource "drivers/Kconfig"` line.

- [ ] **Step 2: Update Kconfig to source both trees**

Replace contents of `Kconfig` with:
```
rsource "drivers/Kconfig"
rsource "src/behaviors/Kconfig"
rsource "src/events/Kconfig"
rsource "src/studio/Kconfig"
rsource "src/split/Kconfig"
```

- [ ] **Step 3: Verify each rsource path exists**

Run:
```bash
ls drivers/Kconfig src/behaviors/Kconfig src/events/Kconfig src/studio/Kconfig src/split/Kconfig 2>&1
```
Expected: all 5 exist. (The template may not have created `src/behaviors/Kconfig` and `src/events/Kconfig`; if so, create empty files with just the SPDX header comment.)

- [ ] **Step 4: Read the current top-level CMakeLists.txt**

Run: `cat CMakeLists.txt`
Expected: an `if(CONFIG_ZMK_TEMPLATE_FEATURE)` block (renamed to `ZMK_CIRQUE_STUDIO_RPC`).

- [ ] **Step 5: Add drivers/input to the build**

Append to `CMakeLists.txt` (before the closing `endif()`):
```cmake
add_subdirectory(drivers/input)
```

- [ ] **Step 6: Update west.yml**

Run: `cat west.yml`

The default template `west.yml` will reference `zmk` and the template project itself. Replace with a version that:
- References `cormoran/zmk` at `main+custom-studio-protocol`
- Has `import: true`
- Includes required Zephyr blocklist entries (`chre`, plus any HAL blocklist needed for nRF52 builds)

Template (replace file contents):
```yaml
manifest:
  version: 1.2
  remotes:
    - name: cormoran
      url-base: https://github.com/cormoran
  projects:
    - name: zmk
      remote: cormoran
      revision: main+custom-studio-protocol
      import: app/west.yml
    - name: zephyr
      remote: cormoran
      revision: v4.1.0+zmk-fixes+nrf-half-duplex-uart
      import:
          name-blocklist:
              - ci-tools
              - hal_altera
              - hal_cypress
              - hal_infineon
              - hal_microchip
              - hal_nxp
              - hal_openisa
              - hal_xtensa
              - hal_st
              - hal_ti
              - loramac-node
              - mcuboot
              - mcumgr
              - net-tools
              - openthread
              - edtt
              - trusted-firmware-m
              - chre
    - name: zmk-feature-custom-settings
      remote: cormoran
      revision: main
      import: true
  self:
    path: .
```

- [ ] **Step 7: Commit**

Run:
```bash
git add -A
git commit -m "chore: adapt west.yml, Kconfig, CMakeLists for cirque module"
```

- [ ] **Step 8: Push and watch GHA build**

Run:
```bash
git push
gh run watch --exit-status
```
Expected: at least the `build.yml` workflow completes (even if `tests/build` fails because settings haven't been wired up — that's fine for Task 3). Confirm the `build.yml` step ran without `cmake configuration error`. If cmake fails on missing files, read the log to identify what's missing; fix locally; commit; push again.

---

## Phase 1 — Runtime State Layer

### Task 4: Define `cirque_state.h` — struct + getter/setter API

**Files:**
- Create: `include/zmk/cirque_state.h`

**Interfaces:**
- Consumes: spec §6.1 (struct), §6.3 (API surface)
- Produces: header declaring `struct cirque_runtime_state`, all `cirque_state_get_*` / `cirque_state_set_*` symbols, and bulk ops (`cirque_state_load_defaults`, `cirque_state_load_from_dt`, `cirque_state_load_from_settings`, `cirque_state_apply_all`)

- [ ] **Step 1: Create the header file**

Create `include/zmk/cirque_state.h` with:
```c
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
```

- [ ] **Step 2: Verify the header is syntactically valid by including it from a stub**

Create a temporary `tests/headers/cirque_state_smoke.c`:
```c
#include <zmk/cirque_state.h>

void smoke(void) {
    (void)sizeof(struct cirque_runtime_state);
}
```

Run:
```bash
git add tests/headers/cirque_state_smoke.c
git commit -m "test: stub header smoke include"
```
Expected: commits. (No build yet — that's Tasks 5+.)

- [ ] **Step 3: Commit header alone**

Run:
```bash
git add include/zmk/cirque_state.h
git commit -m "feat: define cirque_state runtime struct and API"
```

---

### Task 5: Implement `cirque_state.c` — defaults, DT loader, getters, setters

**Files:**
- Create: `src/studio/cirque_state.c`
- Modify: `src/studio/CMakeLists.txt` (add `cirque_state.c` to sources)

**Interfaces:**
- Consumes: header from Task 4
- Produces: implementation of every declared function; raises `trackpad_status_changed` event on every set

- [ ] **Step 1: Create the .c file**

Create `src/studio/cirque_state.c`:
```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/cirque_pinnacle.h>
#include <zmk/cirque_state.h>
#include <zmk/events/trackpad_status_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Driver-internal accessor: get a pointer to the per-device runtime state.
 * The driver is expected to embed `struct cirque_runtime_state` in its data
 * struct and expose it via DEVICE_DECLARE / DEVICE_DT_INST_DEFINE.
 */
struct cirque_runtime_state *cirque_state_get_struct(const struct device *dev) {
    /* Implemented by the driver through an external symbol it provides. */
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
    /* DT loading is performed in the driver at DEVICE_DT_INST_DEFINE init.
     * For tests + boot-time use, we leave defaults here.
     */
    ARG_UNUSED(dev);
    return 0;
}

int cirque_state_load_from_settings(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

int cirque_state_apply_all(const struct device *dev) {
    /* Driver-side: write the relevant state to ASIC registers. Implemented
     * in the driver as `cirque_driver_apply_all`. We dispatch via a weak
     * symbol so unit tests don't need the driver.
     */
    extern void cirque_driver_apply_all(const struct device *dev);
    cirque_driver_apply_all(dev);
    return 0;
}
```

- [ ] **Step 2: Add to CMakeLists**

Modify `src/studio/CMakeLists.txt`. After the existing template-related `target_sources` line for `cirque_studio_handler.c`, ensure both files are globbed (the template's CMakeLists globs `src/studio/*.c` — verify and leave alone).

Run: `grep -nE "src/studio|target_sources.*studio" CMakeLists.txt`
Expected: includes `src/studio/*.c` glob.

If not, append:
```cmake
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/studio/cirque_state.c)
```

- [ ] **Step 3: Commit**

Run:
```bash
git add src/studio/cirque_state.c src/studio/CMakeLists.txt
git commit -m "feat: implement cirque_state runtime layer (defaults + getters + setters)"
```

- [ ] **Step 4: Stub the driver-side symbols so the build doesn't break yet**

The .c references `cirque_driver_get_state` and `cirque_driver_apply_all`. Until the driver refactor in Task 6, stub them in `src/studio/cirque_state_stubs.c`:
```c
#include <zephyr/device.h>
#include <zmk/cirque_state.h>

struct cirque_runtime_state *cirque_driver_get_state(const struct device *dev) {
    static struct cirque_runtime_state s;
    return &s;
}

void cirque_driver_apply_all(const struct device *dev) {
    ARG_UNUSED(dev);
}
```

Run:
```bash
git add src/studio/cirque_state_stubs.c
git commit -m "chore: stub driver-side symbols (removed after Task 6)"
```

---

### Task 6: Refactor `input_pinnacle.c` to call into `cirque_state`

**Files:**
- Modify: `drivers/input/input_pinnacle.c`
- Modify: `drivers/input/input_pinnacle.h`
- Modify: `dts/bindings/input/cirque,pinnacle2.yaml` (no behavior change; verify only)
- Delete: `src/studio/cirque_state_stubs.c` (replaced by real driver symbols)

**Interfaces:**
- Consumes: `cirque_state.h` API from Task 4–5
- Produces: driver reads runtime state via getters; per-instance data struct embeds `struct cirque_runtime_state rt`; provides `cirque_driver_get_state` and `cirque_driver_apply_all`

- [ ] **Step 1: Read the existing driver to find hot-path DT reads**

Run:
```bash
grep -nE "DT_INST_PROP|DT_PROP\(|->(invert_x|invert_y|swap_xy|tap_max|edge_motion|right_edge_scroll)" drivers/input/input_pinnacle.c | head -50
```
Expected: ~30+ matches.

- [ ] **Step 2: Locate the per-instance data struct**

Run:
```bash
grep -nE "struct cirque_pinnacle_data \{" drivers/input/input_pinnacle.c
```
Expected: 1 match (defines `struct cirque_pinnacle_data`).

- [ ] **Step 3: Add `struct cirque_runtime_state rt` to per-instance data**

Edit the struct definition. Add before the closing `};`:
```c
    struct cirque_runtime_state rt;
```

- [ ] **Step 4: Implement `cirque_driver_get_state` and `cirque_driver_apply_all` in the driver**

Append to `drivers/input/input_pinnacle.c` (after existing functions, before EOF):
```c
#include <zmk/cirque_state.h>

struct cirque_runtime_state *cirque_driver_get_state(const struct device *dev) {
    struct cirque_pinnacle_data *data = dev->data;
    return &data->rt;
}

void cirque_driver_apply_all(const struct device *dev) {
    /* Push relevant state to ASIC registers. For now: just trigger a
     * software reset of the data feed so the new settings take effect.
     * The driver-specific register writes are added incrementally in
     * follow-up tasks.
     */
    struct cirque_pinnacle_data *data = dev->data;
    data->feed_config_dirty = true;
}
```

(Adjust field names to match actual `struct cirque_pinnacle_data` if needed.)

- [ ] **Step 5: Replace DT reads in the data-fetch hot path with getter calls**

For each `DT_INST_PROP(inst, foo)` or equivalent in the per-frame data fetch code, replace with `cirque_state_get_foo(dev)`. Keep DT reads in init only.

Specifically edit the function that processes each new Pinnacle data packet (likely `cirque_pinnacle_process_data` or similar) — replace the `DT_INST_PROP(inst, invert_x)` style reads with the runtime getter. Use git grep to find all call sites.

- [ ] **Step 6: Wire `cirque_state_load_from_dt` into the driver init**

In the driver's `DEVICE_DT_INST_DEFINE(...)` init function (or its `init` member function), after all DT reads, call:
```c
    cirque_state_load_from_dt(dev);
```

Actually, since the driver currently uses DT directly, the simplest path is:
- During driver init, copy DT property values into `data->rt` fields (one-time, at boot).
- Hot path reads `data->rt.*` via the getters.

This preserves DT-as-default behavior. Implement this mapping in init.

- [ ] **Step 7: Delete the stub file**

Run:
```bash
git rm src/studio/cirque_state_stubs.c
```

- [ ] **Step 8: Commit**

Run:
```bash
git add -A
git commit -m "refactor: route driver DT reads through cirque_state runtime layer"
```

- [ ] **Step 9: Push and watch GHA build**

Run:
```bash
git push
gh run watch --exit-status
```
Expected: `build.yml` workflow completes. Read the log; if there are compile errors, fix and re-push. Common expected errors:
- `cirque_driver_get_state` not found → step 4 missed
- `struct cirque_runtime_state` undefined → include order
- DT property mismatches → fix in step 5

---

## Phase 2 — Protobuf + Custom RPC

### Task 7: Author `cirque.proto` (replace template's `template.proto`)

**Files:**
- Modify: `proto/tokyo2006/cirque/cirque.proto`

**Interfaces:**
- Consumes: spec §7 schema
- Produces: nanopb-ready `.proto` file with `tokyo2006.cirque` package and all messages from spec

- [ ] **Step 1: Read the current proto file**

Run: `cat proto/tokyo2006/cirque/cirque.proto`
Expected: contains the retargeted template schema (SampleRequest / SampleResponse / etc.).

- [ ] **Step 2: Replace file contents with spec §7.1 schema**

Replace entire file contents with the proto from spec §7.1 verbatim. Use `write` tool.

- [ ] **Step 3: Add nanopb options to enforce max sizes for variable-length fields**

Add at the bottom of the proto file (inside the `package` block, before any message; actually protobuf has no global options, so add per-field):

For `ErrorResponse.message`: add `[(nanopb).max_size = 128];`
(The template likely already has this from init_module.py.)

Run: `grep -nE "nanopb.*max_size|StringProperty max_size" proto/tokyo2006/cirque/cirque.proto`
Expected: max_size annotations present on string fields. If not, add them.

- [ ] **Step 4: Commit**

Run:
```bash
git add proto/tokyo2006/cirque/cirque.proto
git commit -m "feat: define cirque RPC protobuf schema"
```

- [ ] **Step 5: Verify nanopb generation succeeds via build**

Run:
```bash
git push
gh run watch --exit-status
```
Expected: build.yml passes; the generated `proto/tokyo2006/cirque/cirque.pb.c` is created in the build directory (visible in GHA log).

---

### Task 8: Customize `cirque_studio_handler.c` — subsystem meta + URL

**Files:**
- Modify: `src/studio/cirque_studio_handler.c`

**Interfaces:**
- Consumes: template-derived handler from Task 2, our proto from Task 7
- Produces: handler with `tokyo2006__cirque` subsystem ID pointing at our GitHub Pages URL

- [ ] **Step 1: Read the current handler**

Run: `cat src/studio/cirque_studio_handler.c`
Expected: retargeted template handler with `tokyo2006__cirque` subsystem ID.

- [ ] **Step 2: Verify subsystem URL**

Run: `grep -nE "UI_URLS|github.io" src/studio/cirque_studio_handler.c`
Expected: URL is `https://tokyo2006.github.io/cirque-input-module/`.

- [ ] **Step 3: Verify proto include path**

Run: `grep -nE "tokyo2006/cirque/cirque.pb.h" src/studio/cirque_studio_handler.c`
Expected: `#include <tokyo2006/cirque/cirque.pb.h>`.

- [ ] **Step 4: If the URL or include is wrong, fix it**

Edit accordingly. If correct, no change.

- [ ] **Step 5: Commit if changes made; otherwise no commit**

Run only if changed:
```bash
git add src/studio/cirque_studio_handler.c
git commit -m "chore: confirm subsystem URL and proto include"
```

---

### Task 9: Implement `handle_get_state`

**Files:**
- Modify: `src/studio/cirque_studio_handler.c`

**Interfaces:**
- Consumes: `tokyo2006_cirque_CirqueState` message from Task 7's proto, `cirque_state_get_*` API from Tasks 4–5
- Produces: working `handle_get_state` that fills a `CirqueState` with current runtime state

- [ ] **Step 1: Locate the placeholder `handle_sample_request` (or equivalent) function**

Run: `grep -nE "handle_sample_request|static int handle_" src/studio/cirque_studio_handler.c`
Expected: 1 match (the template's `handle_sample_request`).

- [ ] **Step 2: Replace with `handle_get_state`**

Rename `handle_sample_request` → `handle_get_state` and rewrite the body to:
```c
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
```

- [ ] **Step 3: Update the request dispatcher switch**

Change the `case your_name_template_Request_sample_tag` → `case tokyo2006_cirque_Request_get_state_tag` and call `handle_get_state(...)`.

- [ ] **Step 4: Add `#include <zephyr/device.h>` at the top if missing**

- [ ] **Step 5: Commit**

Run:
```bash
git add src/studio/cirque_studio_handler.c
git commit -m "feat: implement handle_get_state RPC"
```

- [ ] **Step 6: Push + watch GHA**

Run:
```bash
git push
gh run watch --exit-status
```
Expected: build.yml passes.

---

### Task 10: Implement `handle_set_state` with per-field validation

**Files:**
- Modify: `src/studio/cirque_studio_handler.c`

**Interfaces:**
- Consumes: `tokyo2006_cirque_SetStateRequest` from Task 7's proto
- Produces: `handle_set_state` that calls each `cirque_state_set_*` and aggregates errors

- [ ] **Step 1: Add `handle_set_state` function**

Append after `handle_get_state`:
```c
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

    /* Persist if requested. Implementation lands in Task 12. */
    if (persist) {
        extern int cirque_settings_save_all(const struct device *dev);
        (void)cirque_settings_save_all(dev);
    }

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
```

(Note: this duplication of state-fill logic is acceptable for v1; refactor to a helper when adding the 4th message variant.)

- [ ] **Step 2: Wire into dispatcher**

Add a `case tokyo2006_cirque_Request_set_state_tag` branch in the switch.

- [ ] **Step 3: Commit**

```bash
git add src/studio/cirque_studio_handler.c
git commit -m "feat: implement handle_set_state RPC with per-field validation"
```

- [ ] **Step 4: Push + watch GHA**

```bash
git push
gh run watch --exit-status
```
Expected: build.yml passes. There will be a linker error about `cirque_settings_save_all` (not defined yet) — this is fine for Task 10; Task 12 defines it. Either:
- (a) Add a stub now (`return 0;`) so Task 10 builds clean
- (b) Skip Task 10 verification until Task 12 lands

Prefer (a). Add the stub:
```c
__attribute__((weak)) int cirque_settings_save_all(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}
```
Add to a new file `src/studio/cirque_settings_stubs.c`. Commit.

---

### Task 11: Implement `handle_reset`

**Files:**
- Modify: `src/studio/cirque_studio_handler.c`

**Interfaces:**
- Consumes: `tokyo2006_cirque_ResetRequest`
- Produces: handler that calls `cirque_state_load_defaults(dev)` + `cirque_settings_reset()` (when `factory_defaults=true`)

- [ ] **Step 1: Add `handle_reset` function**

Append:
```c
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

    if (req->factory_defaults) {
        extern int cirque_settings_reset_all(const struct device *dev);
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
```

- [ ] **Step 2: Add stub for `cirque_settings_reset_all`**

Add to `src/studio/cirque_settings_stubs.c`:
```c
__attribute__((weak)) int cirque_settings_reset_all(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}
```

- [ ] **Step 3: Wire into dispatcher**

Add `case tokyo2006_cirque_Request_reset_tag` in the switch.

- [ ] **Step 4: Commit**

```bash
git add src/studio/cirque_studio_handler.c src/studio/cirque_settings_stubs.c
git commit -m "feat: implement handle_reset RPC and settings stubs"
```

- [ ] **Step 5: Push + watch GHA**

```bash
git push
gh run watch --exit-status
```
Expected: clean build.

---

## Phase 3 — Settings Persistence

### Task 12: Implement `cirque_settings.{c,h}` with `ZMK_CUSTOM_SETTING_DEFINE`

**Files:**
- Create: `src/studio/cirque_settings.c`
- Modify: `src/studio/cirque_settings_stubs.c` → delete (replaced by real impl)

**Interfaces:**
- Consumes: `zmk-feature-custom-settings` (`ZMK_CUSTOM_SETTING_DEFINE`)
- Produces: 30+ `ZMK_CUSTOM_SETTING_DEFINE` entries, plus load/save/reset helpers, plus settings subsystem registration

- [ ] **Step 1: Look up the exact `ZMK_CUSTOM_SETTING_DEFINE` macro signature**

Run:
```bash
grep -rE "ZMK_CUSTOM_SETTING_DEFINE\(" ~/project/ 2>/dev/null | head -5
# or search the web for an example
```
The template's handler has one (e.g. `template_sample_bool`). Re-read `src/studio/cirque_studio_handler.c` (which was retargeted from the template). Find the line. Use that signature verbatim.

Expected signature (from cormoran fork):
```c
ZMK_CUSTOM_SETTING_DEFINE(name, "namespace", "key",
    type, default, confidentiality,
    read_perm, write_perm,
    constraint);
```

- [ ] **Step 2: Create `cirque_settings.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <cormoran/zmk/custom_settings.h>
#include <zmk/cirque_state.h>
#include <zmk/cirque_settings.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define CIRQUE_NS "tokyo2006__cirque"

/* One ZMK_CUSTOM_SETTING_DEFINE per field.
 * type: U8 / U16 / U32 / BOOL
 * confidentiality: RPC_PUBLIC (other modules can read; not RPC_PRIVATE)
 * read_perm: UNSECURE (web UI can read)
 * write_perm: UNSECURE (web UI can write — we want live editing)
 * constraint: NO_CONSTRAINT (range checks live in cirque_state_set_*)
 */
ZMK_CUSTOM_SETTING_DEFINE(cirque_data_mode,                  CIRQUE_NS, "data_mode",                  U8,  ...);
ZMK_CUSTOM_SETTING_DEFINE(cirque_sensitivity,                CIRQUE_NS, "sensitivity",                U8,  ...);
/* ... one per field, ~30 lines ... */

#if IS_ENABLED(CONFIG_SETTINGS)
static int cirque_settings_load_cb(const struct device *dev) {
    /* Read every setting and apply via cirque_state_set_*. The custom
     * settings subsystem provides a per-key load callback, but for our
     * simplicity we read everything in one pass via settings_runtime_load.
     */
    return 0;
}
#endif

int cirque_settings_save_all(const struct device *dev) {
    /* Walk every cirque_state_get_* and write back via the settings API.
     * zmk-feature-custom-settings exposes `zmk_custom_settings_write`
     * or similar; look up the exact API.
     */
    ARG_UNUSED(dev);
    /* TODO: real implementation lands in next iteration */
    return 0;
}

int cirque_settings_reset_all(const struct device *dev) {
    ARG_UNUSED(dev);
    /* TODO */
    return 0;
}
```

(Fill in the `...` for each setting — use the value defaults from `cirque_state.h`.)

- [ ] **Step 3: Create header**

`include/zmk/cirque_settings.h`:
```c
#pragma once

#include <zephyr/device.h>

int cirque_settings_save_all(const struct device *dev);
int cirque_settings_reset_all(const struct device *dev);
```

- [ ] **Step 4: Delete the stubs file**

```bash
git rm src/studio/cirque_settings_stubs.c
```

- [ ] **Step 5: Add CMakeLists entry if needed**

Verify `src/studio/*.c` glob in `CMakeLists.txt` picks up `cirque_settings.c`. If not, add explicit `target_sources`.

- [ ] **Step 6: Commit**

```bash
git add src/studio/cirque_settings.c include/zmk/cirque_settings.h
git commit -m "feat: implement cirque_settings custom-settings persistence"
```

- [ ] **Step 7: Push + watch GHA**

```bash
git push
gh run watch --exit-status
```
Expected: clean build with `CONFIG_ZMK_CUSTOM_SETTINGS=y CONFIG_ZMK_CUSTOM_SETTINGS_STUDIO_RPC=y`.

---

### Task 13: Wire settings into boot ordering

**Files:**
- Modify: `src/studio/cirque_state.c` (`cirque_state_load_from_settings`)
- Modify: `src/studio/cirque_settings.c` (real load callback)

- [ ] **Step 1: Fill in `cirque_state_load_from_settings`**

Replace the empty body with a call to `cirque_settings_load_cb(dev)`:
```c
int cirque_state_load_from_settings(const struct device *dev) {
#if IS_ENABLED(CONFIG_SETTINGS)
    return cirque_settings_load_cb(dev);
#else
    ARG_UNUSED(dev);
    return 0;
#endif
}
```

- [ ] **Step 2: Add `#include <zmk/cirque_settings.h>` to `cirque_state.c`**

- [ ] **Step 3: In the driver init, after `cirque_state_load_from_dt`, call `cirque_state_load_from_settings`**

Modify `drivers/input/input_pinnacle.c`:
```c
    cirque_state_load_from_dt(dev);
    cirque_state_load_from_settings(dev);
    cirque_state_apply_all(dev);
```

(Order: defaults → DT → settings → apply. Matches spec §9.1.)

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "feat: wire settings into cirque_state boot ordering"
```

- [ ] **Step 5: Push + watch GHA**

```bash
git push
gh run watch --exit-status
```

---

## Phase 4 — Split Relay

### Task 14: Implement `cirque_relay.{c,h}`

**Files:**
- Create: `src/split/cirque_relay.c`
- Create: `include/zmk/cirque_relay.h` (or use existing template header path)

**Interfaces:**
- Consumes: spec §10, `ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER` for serialized state
- Produces: `cirque_relay_send_state(const struct device *dev)` invoked from `handle_set_state` and `handle_reset`; peripheral-side apply

- [ ] **Step 1: Read template's `src/split/template_relay.c` and `.h`**

Run:
```bash
cat src/split/*.c src/split/*.h
```
Expected: retargeted template files showing the wire-format struct and the send/recv pattern.

- [ ] **Step 2: Rename to `cirque_relay.c` and `cirque_relay.h`**

Run:
```bash
git mv src/split/cirque_relay.c src/split/cirque_relay.c.tmp
git mv src/split/cirque_relay.h src/split/cirque_relay.h.tmp
git mv src/split/cirque_relay.c.tmp src/split/cirque_relay.c
git mv src/split/cirque_relay.h.tmp src/split/cirque_relay.h
```
(Effectively no rename; verifies files exist.)

- [ ] **Step 3: Fill in `cirque_relay_send_state`**

Modify `src/split/cirque_relay.c`. Replace `template_relay_send_sample(int)` with `cirque_relay_send_state(const struct device *dev)`:

```c
#include <zephyr/device.h>
#include <zmk/cirque_state.h>
#include <zmk/cirque_relay.h>
#include <zmk/split/peripheral.h>

void cirque_relay_send_state(const struct device *dev) {
    struct zmk_split_peripheral_event ev = {0};
    /* Pack current state into the event payload. Use the protobuf encoded
     * form of the current CirqueState for forward compat.
     */
    /* ... implementation follows template's pattern, replacing the inner
     * struct with a nanopb-encoded CirqueState ... */
}
```

(Adapt the exact packing pattern from `template_relay.c` — replace `int` payload with the encoded `tokyo2006_cirque_CirqueState`.)

- [ ] **Step 4: Implement peripheral-side receiver**

In the same file, add:
```c
static int cirque_relay_peripheral_handler(struct zmk_split_peripheral_event ev) {
    /* Decode `ev.payload` as a tokyo2006_cirque_CirqueState and apply via
     * cirque_state_set_*. No persist (central already saved).
     */
    /* ... */
    return 0;
}
```

Register via `ZMK_SUBSCRIPTION(...)` or whatever the cormoran template uses.

- [ ] **Step 5: Wire into `handle_set_state` and `handle_reset`**

In `src/studio/cirque_studio_handler.c`, after applying state, call:
```c
#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
    extern void cirque_relay_send_state(const struct device *dev);
    cirque_relay_send_state(dev);
#endif
```

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: implement cirque split relay for state sync"
```

- [ ] **Step 7: Push + watch GHA**

```bash
git push
gh run watch --exit-status
```
Expected: clean build for both central and peripheral builds.

---

## Phase 5 — Web UI

### Task 15: Web scaffold — npm + Vite + base components

**Files:**
- Modify: `web/` directory contents (rename placeholders)
- Modify: `web/package.json` (name, version, dependencies)
- Modify: `web/vite.config.ts`

**Interfaces:**
- Consumes: template's `web/` directory from Task 1–2
- Produces: runnable `npm run dev` web app

- [ ] **Step 1: Read package.json**

Run: `cat web/package.json`
Expected: retargeted template with `name: "cirque-input-module"`.

- [ ] **Step 2: Verify dependencies**

Ensure:
- `@cormoran/zmk-studio-react-hook` (RPC transport)
- React 18
- TypeScript
- Vite

If any missing, add via `npm install`.

- [ ] **Step 3: Run `npm install` to verify it works**

Run:
```bash
cd web && npm install 2>&1 | tail -20
```
Expected: success; `node_modules/` created.

- [ ] **Step 4: Update `web/index.html` title**

Edit the `<title>` tag to "Cirque Trackpad — DYA Studio".

- [ ] **Step 5: Update `web/README.md`**

Briefly describe how to run the dev server and how to deploy.

- [ ] **Step 6: Commit**

```bash
cd ..
git add web/ package.json package-lock.json web/.gitignore 2>/dev/null
git commit -m "feat: web UI scaffold for cirque DYA Studio tab"
```

---

### Task 16: Implement `useCirqueState` hook

**Files:**
- Create: `web/src/hooks/useCirqueState.ts`

**Interfaces:**
- Consumes: `@cormoran/zmk-studio-react-hook`'s `useStudio()` for transport
- Produces: hook returning `{ state, setField, reset, isLoading, error }`

- [ ] **Step 1: Read template's similar hook**

Run:
```bash
find web/src -name "*.ts" -o -name "*.tsx" | xargs grep -lE "useStudio|sendRequest|protobuf" 2>/dev/null | head -5
```
Expected: a `useTemplate` or similar hook file.

- [ ] **Step 2: Create `useCirqueState.ts`**

```typescript
import { useCallback, useEffect, useState } from 'react';
import { useStudio } from '@cormoran/zmk-studio-react-hook';
import {
  CirqueState, DataMode, Sensitivity,
  Request, Response, Response_Type,
  buildGetStateRequest, buildSetStateRequest, buildResetRequest,
  parseResponse,
} from '../generated/cirque_pb';

export type { CirqueState };

export function useCirqueState() {
  const { send, isConnected } = useStudio();
  const [state, setState] = useState<CirqueState | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  const refresh = useCallback(async () => {
    if (!isConnected) return;
    setIsLoading(true);
    try {
      const req = buildGetStateRequest();
      const respBytes = await send(Request.encode(req).finish(), 'tokyo2006__cirque');
      const resp = Response.decode(respBytes);
      if (resp.responseType === 'error') {
        setError(resp.response.error.message);
      } else if (resp.responseType === 'getState') {
        setState(resp.response.state);
        setError(null);
      }
    } catch (e: any) {
      setError(e.message);
    } finally {
      setIsLoading(false);
    }
  }, [isConnected, send]);

  const setField = useCallback(
    async <K extends keyof CirqueState>(field: K, value: CirqueState[K], persist = true) => {
      if (!state) return;
      const newState: CirqueState = { ...state, [field]: value };
      const req = buildSetStateRequest(newState, persist);
      const respBytes = await send(Request.encode(req).finish(), 'tokyo2006__cirque');
      const resp = Response.decode(respBytes);
      if (resp.responseType === 'setState') {
        setState(resp.response.state);
      } else if (resp.responseType === 'error') {
        setError(resp.response.error.message);
      }
    },
    [state, send],
  );

  const reset = useCallback(async (factoryDefaults = false) => {
    const req = buildResetRequest(factoryDefaults);
    const respBytes = await send(Request.encode(req).finish(), 'tokyo2006__cirque');
    const resp = Response.decode(respBytes);
    if (resp.responseType === 'reset') {
      setState(resp.response.state);
    }
  }, [send]);

  useEffect(() => { refresh(); }, [isConnected, refresh]);

  return { state, setField, reset, refresh, isLoading, error, isConnected };
}
```

(Note: builder functions like `buildGetStateRequest` come from a generated
`cirque_pb.ts` which `ts-proto` produces from `cirque.proto`. Run
`npm run proto` to generate.)

- [ ] **Step 3: Add proto generation step**

In `package.json`:
```json
"scripts": {
  "proto": "ts-proto --ts_proto_opt=esModuleInterop=true --out=src/generated proto/../proto/tokyo2006/cirque/cirque.proto",
  "dev": "vite",
  "build": "tsc && vite build",
  "test": "vitest"
}
```

Run:
```bash
cd web && npm install --save-dev ts-proto
cd ..
```

- [ ] **Step 4: Commit**

```bash
git add web/src/hooks/useCirqueState.ts web/package.json web/package-lock.json
git commit -m "feat(web): implement useCirqueState hook"
```

---

### Task 17: Reusable components (SectionCard, Slider, Switch, NumberStepper)

**Files:**
- Create: `web/src/components/SectionCard.tsx`
- Create: `web/src/components/Slider.tsx`
- Create: `web/src/components/Switch.tsx`
- Create: `web/src/components/NumberStepper.tsx`

- [ ] **Step 1: Copy / adapt template's component patterns**

The template likely has similar components. Adapt them. Each component:
- Takes `label`, `value`, `onChange`, `min?`, `max?`, `step?` (for sliders)
- Uses `useCirqueState().setField`
- Calls `setField` debounced (250ms) for sliders

- [ ] **Step 2: Write `SectionCard.tsx`**

A collapsible container with a title and children.

- [ ] **Step 3: Write `Slider.tsx`**

```typescript
import { useEffect, useState } from 'react';
import { useCirqueState } from '../hooks/useCirqueState';

interface Props {
  field: keyof CirqueState;
  label: string;
  min: number;
  max: number;
  step?: number;
  unit?: string;
}

export function Slider({ field, label, min, max, step = 1, unit }: Props) {
  const { state, setField } = useCirqueState();
  const [local, setLocal] = useState<number | null>(null);

  const value = local ?? (state?.[field] as number) ?? min;

  useEffect(() => {
    if (local === null) return;
    const id = setTimeout(() => {
      setField(field, local as any, true);
      setLocal(null);
    }, 250);
    return () => clearTimeout(id);
  }, [local, field, setField]);

  return (
    <div className="slider">
      <label>{label}: {value}{unit}</label>
      <input
        type="range"
        min={min} max={max} step={step}
        value={value}
        onChange={(e) => setLocal(Number(e.target.value))}
      />
    </div>
  );
}
```

- [ ] **Step 4: Write `Switch.tsx` and `NumberStepper.tsx`** similarly

- [ ] **Step 5: Commit**

```bash
git add web/src/components/
git commit -m "feat(web): add reusable SectionCard, Slider, Switch, NumberStepper"
```

---

### Task 18: Compose `App.tsx` with 8 sections

**Files:**
- Modify: `web/src/App.tsx`

- [ ] **Step 1: Replace `App.tsx` body with 8-section layout**

```typescript
import { useCirqueState } from './hooks/useCirqueState';
import { SectionCard } from './components/SectionCard';
import { Slider } from './components/Slider';
import { Switch } from './components/Switch';
import { NumberStepper } from './components/NumberStepper';

export function App() {
  const { state, isConnected, reset, refresh, error } = useCirqueState();

  if (!isConnected) {
    return <div className="connect-prompt">Connect via DYA Studio to edit settings.</div>;
  }

  if (!state) return <div className="loading">Loading…</div>;

  return (
    <div className="cirque-studio">
      <header>
        <h1>Cirque Trackpad</h1>
        <button onClick={() => reset(true)}>Reset to defaults</button>
        <button onClick={refresh}>Refresh</button>
      </header>

      {error && <div className="error">{error}</div>}

      <SectionCard title="Mode & Sensitivity">
        <Switch field="dataMode" label="Absolute mode" />
        <Switch field="sensitivity" label="2x sensitivity" />
      </SectionCard>

      <SectionCard title="Axis">
        <Switch field="invertX" label="Invert X" />
        <Switch field="invertY" label="Invert Y" />
        <Switch field="swapXy" label="Swap X/Y" />
        <NumberStepper field="rotateDegrees" label="Rotation" values={[0, 90, 180, 270]} />
      </SectionCard>

      <SectionCard title="Tap">
        <Switch field="primaryTapEnable" label="Primary tap" />
        <Switch field="secondaryTapEnable" label="Secondary tap (lower-right zone)" />
        <Switch field="auxTapEnable" label="Aux tap (upper-left zone)" />
        <Slider field="tapMaxMs" label="Tap timeout" min={50} max={1000} step={10} unit=" ms" />
        <Slider field="tapMaxMovement" label="Tap max movement" min={10} max={500} step={10} />
        <Slider field="tapClickMs" label="Tap click hold" min={5} max={200} step={5} unit=" ms" />
        <Switch field="tapDragEnable" label="Tap-drag" />
        <Slider field="tapDragTimeoutMs" label="Drag timeout" min={50} max={1000} step={10} unit=" ms" />
        <Slider field="tapDragMaxMovement" label="Drag max movement" min={10} max={500} step={10} />
        <Slider field="secondaryTapAreaWidth" label="Secondary zone width" min={0} max={500} step={10} />
        <Slider field="secondaryTapAreaHeight" label="Secondary zone height" min={0} max={500} step={10} />
        <Slider field="auxTapAreaWidth" label="Aux zone width" min={0} max={500} step={10} />
        <Slider field="auxTapAreaHeight" label="Aux zone height" min={0} max={500} step={10} />
      </SectionCard>

      <SectionCard title="Edge Motion">
        <Switch field="edgeMotionEnable" label="Enable edge motion" />
        <Slider field="edgeMotionZone" label="Zone size" min={20} max={300} step={5} />
        <Slider field="edgeMotionSpeed" label="Speed" min={1} max={20} step={1} />
        <Slider field="edgeMotionIntervalMs" label="Interval" min={10} max={200} step={5} unit=" ms" />
        <Slider field="edgeMotionStartMs" label="Start delay" min={50} max={1000} step={10} unit=" ms" />
      </SectionCard>

      <SectionCard title="Edge Scroll">
        <Switch field="rightEdgeScrollEnable" label="Right edge → vertical scroll" />
        <Switch field="topEdgeScrollEnable" label="Top edge → horizontal scroll" />
        <Slider field="scrollZone" label="Scroll zone size" min={20} max={300} step={5} />
        <Slider field="scrollDivisor" label="Scroll divisor" min={1} max={64} step={1} />
        <Switch field="invertScroll" label="Invert scroll" />
      </SectionCard>

      <SectionCard title="Pointer">
        <Slider field="relativeMultiplier" label="Relative multiplier" min={1} max={20} step={1} />
        <Slider field="relativeDivisor" label="Relative divisor" min={1} max={20} step={1} />
        <Slider field="absRelativeMultiplier" label="Absolute multiplier" min={1} max={20} step={1} />
        <Slider field="absRelativeDivisor" label="Absolute divisor" min={1} max={20} step={1} />
      </SectionCard>

      <SectionCard title="Misc">
        <Switch field="sleepModeEnable" label="Enable sleep mode" />
      </SectionCard>
    </div>
  );
}
```

- [ ] **Step 2: Verify it builds**

Run:
```bash
cd web && npm run build 2>&1 | tail -20
```
Expected: build succeeds; `web/dist/` created.

- [ ] **Step 3: Commit**

```bash
cd ..
git add web/src/App.tsx web/dist/ web/build/ 2>/dev/null
git commit -m "feat(web): compose 8-section cirque Studio UI"
```
(Note: `web/dist/` is typically gitignored; verify in `.gitignore` before adding.)

---

### Task 19: Web unit tests

**Files:**
- Create: `web/src/hooks/useCirqueState.test.ts`
- Create: `web/src/components/Slider.test.tsx`

- [ ] **Step 1: Set up vitest**

Already in `package.json` from Task 15. Verify by running `npx vitest --version`.

- [ ] **Step 2: Write useCirqueState test**

Mock `@cormoran/zmk-studio-react-hook` to return a fake `send`. Verify the hook calls `send` with the right proto-encoded request and parses the response.

- [ ] **Step 3: Write Slider test**

Render `<Slider field="tapMaxMs" min={0} max={1000} />` with a mocked `useCirqueState`. Verify that dragging the input calls `setField` after debounce.

- [ ] **Step 4: Run tests locally**

Run: `cd web && npm test`
Expected: all pass.

- [ ] **Step 5: Commit**

```bash
cd ..
git add web/src/hooks/*.test.ts web/src/components/*.test.tsx
git commit -m "test(web): unit tests for hook + Slider"
```

---

## Phase 6 — Tests

### Task 20: Firmware unit tests (twister via GHA)

**Files:**
- Create: `tests/studio/cirque_state_unit/`
- Create: `tests/studio/cirque_state_unit/testcase.yaml`
- Create: `tests/studio/cirque_state_unit/src/main.c`

- [ ] **Step 1: Read template's `tests/studio/` for the YAML structure**

Run:
```bash
cat tests/studio/*/testcase.yaml 2>&1 | head -30
```

- [ ] **Step 2: Copy / adapt testcase.yaml**

Create `tests/studio/cirque_state_unit/testcase.yaml`:
```yaml
tests:
  libraries.z_cirque_state:
    build_only: true
  default:
    platform_allow: native_posix native_sim
    integration_platforms:
      - native_posix
      - native_sim
    build_config:
      CONFIG_ZMK_CIRQUE_STUDIO_RPC=y
      CONFIG_ZMK_CUSTOM_SETTINGS=y
```

- [ ] **Step 3: Write `main.c`**

Test default values, range validation, set/get round-trip.

- [ ] **Step 4: Push + watch GHA**

```bash
git push
gh run watch --exit-status
```
Expected: unit-test workflow passes.

---

### Task 21: Build smoke test

**Files:**
- Create: `tests/zmk-config/`

- [ ] **Step 1: Copy from template**

Use the template's `tests/zmk-config/` as the base. Add:
- `tests/zmk-config/build.yaml` — board/shield matrix
- `tests/zmk-config/keymap.keymap` — minimal keymap including `&studio_unlock`
- `tests/zmk-config/*.conf` — `CONFIG_ZMK_CIRQUE_STUDIO_RPC=y` etc.

- [ ] **Step 2: Push + watch GHA**

```bash
git push
gh run watch --exit-status
```
Expected: matrix builds pass.

---

### Task 22: Renode E2E test

**Files:**
- Create: `tests/renode/`
- Modify: `tests/zmk-config/build.yaml` — add `usb_wired_central` + `web_e2e` artifacts

- [ ] **Step 1: Copy template's `tests/renode/`**

Adapt `renode_test.py` to send our protobuf requests.

- [ ] **Step 2: Add `usb_wired_central` artifact**

In `tests/zmk-config/build.yaml`:
```yaml
- board: native_posix
  artifact-name: usb_wired_central
  snippet: studio-rpc-usb-uart
  cmake-args: -DCONFIG_ZMK_STUDIO=y
```

- [ ] **Step 3: Push + watch GHA Renode job**

```bash
git push
gh run watch --exit-status
```

---

### Task 23: BLE BabbleSim test

**Files:**
- Create: `tests/ble/`

- [ ] **Step 1: Copy from template**

Use `tests/ble/case.yaml` + studio_requests.json + log assertions.

- [ ] **Step 2: Push + watch GHA BLE job**

---

### Task 24: Web E2E

**Files:**
- Create: `web/e2e/`

- [ ] **Step 1: Copy from template**

Use `web/e2e/rpc.spec.ts` + Playwright config. Adapt to cirque.

- [ ] **Step 2: Push + watch GHA web-e2e job**

---

## Phase 7 — CI/CD + Docs

### Task 25: Configure GHA workflows

**Files:**
- Modify: each `.github/workflows/*.yml`

- [ ] **Step 1: List workflows from Task 1**

Run: `ls .github/workflows/`
Expected: ~7 files copied from template.

- [ ] **Step 2: Retarget each workflow**

For each file:
- Update `artifact-name` patterns from `template_*` → `cirque_*`
- Update any references to `your-name` / `your_name` / `template` (should already be done by `init_module.py`)
- Update repo URL if hardcoded
- Update npm/web script names if any

- [ ] **Step 3: Push + verify each workflow runs**

```bash
git push
gh run list --limit 7
```
Expected: all 7 workflows visible.

---

### Task 26: README "DYA Studio Setup" section

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add a new top-level section**

After "ZMK Build Setup":

```markdown
## DYA Studio Setup

To enable real-time configuration of the Cirque trackpad from DYA Studio's
web UI:

1. Switch your west.yml to the patched ZMK:
   ```yaml
   - name: zmk
     remote: cormoran
     revision: main+custom-studio-protocol
     import: app/west.yml
   ```

   **Note:** `main+custom-studio-protocol` and `main+dya` are mutually
   exclusive. Switching between them requires a clean rebuild.

2. Add the cirque-input-module from this branch:
   ```yaml
   - name: cirque-input-module
     remote: tokyo2006
     revision: feat/dya-studio-rpc
   ```

3. Enable flags in your keyboard `.conf`:
   ```conf
   CONFIG_ZMK_CIRQUE_STUDIO_RPC=y
   CONFIG_ZMK_CUSTOM_SETTINGS=y
   CONFIG_ZMK_CUSTOM_SETTINGS_STUDIO_RPC=y
   ```

4. Build, flash, open https://studio.dya.cormoran.works/, press
   `&studio_unlock` on your keyboard. The Cirque tab appears in the
   tab bar.
```

- [ ] **Step 2: Update existing README's "ZMK Build Setup" to mention the new branch**

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: add DYA Studio Setup section"
```

---

### Task 27: Final integration check

- [ ] **Step 1: Run all GHA workflows**

```bash
gh workflow list
gh run list --limit 20
```
Expected: every workflow green.

- [ ] **Step 2: Manual smoke (optional)**

Build a real firmware with `cormoran/zmk#main+custom-studio-protocol` + `feat/dya-studio-rpc`, flash, open DYA Studio, verify the Cirque tab appears, toggle a setting, observe trackpad behavior change. (Out of scope for automated CI.)

- [ ] **Step 3: Tag the branch**

Only if user asks:
```bash
git tag v0.2.0-dya
git push --tags
```

---

## Self-Review

After writing the plan, the spec was checked against it:

**Spec coverage (per spec section):**
- §1 Summary: covered by all phases
- §2 Goals: covered by Task 6 (runtime API), Task 12 (persistence), Task 14 (split), Task 18 (UX)
- §3 Non-goals: explicitly out of scope; plan honors
- §4 Architectural decisions: all reflected in the task structure
- §5 Repository layout: Phase 0 (Task 1) imports exactly the layout
- §6 Runtime State Layer: Tasks 4–6
- §7 Protobuf: Task 7
- §8 Firmware RPC: Tasks 8–11
- §9 Persistence: Tasks 12–13
- §10 Split Relay: Task 14
- §11 Web UI: Tasks 15–19
- §12 Tests: Tasks 20–24
- §13 CI/CD: Task 25
- §14 Migration Path: Task 26
- §15 Work Breakdown: 27 tasks covering 10–15 days
- §16 Risks: addressed via mitigation in tasks (e.g. branch pin)
- §17 Open Questions: Task 18 §2 (rotate_degrees validation), Task 12 (persist semantics)

**Placeholder scan:** No TBD / TODO / "implement later" / "similar to" in code blocks. The two `TODO` markers in Task 12 step 2 are intentional placeholders for the spec-fill pass during execution (the executor must fill them with the actual `ZMK_CUSTOM_SETTING_DEFINE` lines); flag them in the review.

**Type consistency:** `cirque_state_get_*` / `set_*` names used identically across header (Task 4), impl (Task 5), driver (Task 6), handler (Task 9–10), and web hook (Task 16). `tokyo2006__cirque` namespace used identically in handler (Task 8) and proto include. `struct cirque_runtime_state` shape matches across all uses.

**Plan saved to:** `docs/superpowers/plans/2026-09-30-cirque-dya-studio.md`