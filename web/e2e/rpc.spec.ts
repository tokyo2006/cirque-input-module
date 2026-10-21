/**
 * End-to-end: this web UI, in a real browser, against this module's real
 * firmware -- with no hardware.
 *
 * The firmware runs in the Renode emulator, booted by zmk-west-commands'
 * `west zmk-web-e2e`, which serves the DUT's ZMK Studio RPC (carried over its
 * emulated USB CDC) to the browser and hands us a `navigator.serial` shim at
 * $ZMK_WEB_E2E_SHIM_URL. Installing that shim is the only thing faked here: the
 * app, its transport, the RPC framing and the firmware are all real.
 *
 *   west zmk-build tests/zmk-config -af web_e2e
 *   west zmk-web-e2e --elf build/web_e2e/zephyr/zmk.elf -- npm --prefix web run e2e
 */
import { test, expect } from "@playwright/test";

const SHIM_URL = process.env.ZMK_WEB_E2E_SHIM_URL;

test("the web UI connects to real firmware and round-trips the cirque RPC", async ({
  page,
  request,
}) => {
  test.skip(
    !SHIM_URL,
    "no DUT: run this through `west zmk-web-e2e` (see the file header)"
  );

  // Install the navigator.serial shim before the app's own scripts run, so the
  // app sees a serial port -- the DUT's Studio CDC in Renode -- to connect to.
  await page.addInitScript(await (await request.get(SHIM_URL!)).text());
  await page.goto("/");

  // Click the app's real Connect button. Its transport opens the shimmed port,
  // completes the Studio handshake against the firmware, and the app renders
  // the name the firmware reported.
  await page.getByRole("button", { name: /Connect USB/ }).click();
  await expect(page.getByText(/Connected to:/)).toBeVisible();

  // The firmware registered this module's custom subsystem, so the cirque
  // settings panel rendered its sections (the app shows a warning otherwise).
  await expect(
    page.getByRole("button", { name: "Mode & Sensitivity" })
  ).toBeVisible();
  await expect(page.getByRole("button", { name: "Axis" })).toBeVisible();

  // The module's own RPC, end to end: toggle "Invert X" and confirm the UI
  // reflects the new value once the firmware's SetState response comes back.
  const invertX = page.getByRole("checkbox", { name: /Invert X/ });
  await expect(invertX).not.toBeChecked();
  await invertX.click();
  await expect(invertX).toBeChecked();
});
