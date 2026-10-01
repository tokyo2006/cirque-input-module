import "./App.css";
import { connect as gattConnect } from "@zmkfirmware/zmk-studio-ts-client/transport/gatt";
import {
  ZMKConnection,
  isWebSerialSupported,
  isWebBluetoothSupported,
  connectSerial,
} from "@cormoran/zmk-studio-react-hook";
import { useCirqueState } from "./hooks/useCirqueState";
import { SectionCard } from "./components/SectionCard";
import { Slider } from "./components/Slider";
import { Switch } from "./components/Switch";
import { NumberStepper } from "./components/NumberStepper";
import { DataMode, Sensitivity } from "./hooks/cirqueTypes";

// Template placeholder: `scripts/init_module.py` rewrites this literal to
// `{owner}/{repo}`. Never write the full
// `...-with-custom-studio-rpc` repo name in a URL built from this constant --
// the replacement targets this exact string first, which would otherwise
// leave the owner unreplaced.
export const GITHUB_REPO = "tokyo2006/cirque-input-module";

// Unlike GITHUB_REPO above, this always credits the original template
// project, regardless of which repo this module was forked into. The
// trailing comment is scripts/init_module.py's IGNORE_MARKER: it keeps this
// line from being rewritten (like GITHUB_REPO is) or flagged as a leftover
// placeholder once initialized.
export const TEMPLATE_CREDIT_REPO = "cormoran/zmk-module-template"; // zmk-module-template:keep

function App() {
  return (
    <div className="app">
      <header className="app-header">
        <h1>Cirque Trackpad — DYA Studio</h1>
        <p>Custom Studio RPC Demo</p>
      </header>

      <ZMKConnection
        autoReconnect
        renderDisconnected={({ connect, isLoading, error }) => (
          <section className="card">
            <h2>Device Connection</h2>
            {isLoading && <p>⏳ Connecting...</p>}
            {error && (
              <div className="error-message">
                <p>🚨 {error}</p>
              </div>
            )}
            {!isLoading && (
              <>
                <div className="connect-buttons">
                  {isWebSerialSupported() && (
                    <button
                      className="btn btn-primary"
                      onClick={() => connect(connectSerial)}
                    >
                      🔌 Connect USB
                    </button>
                  )}
                  {isWebBluetoothSupported() && (
                    <button
                      className="btn btn-primary"
                      onClick={() => connect(gattConnect)}
                    >
                      📶 Connect Bluetooth
                    </button>
                  )}
                  {!isWebSerialSupported() && !isWebBluetoothSupported() && (
                    <div className="warning-message">
                      <p>
                        ⚠️ Web Serial and Web Bluetooth are unavailable here.
                        Use a Chromium-based browser (Chrome, Edge, ...) over
                        HTTPS or localhost to connect to your keyboard.
                      </p>
                    </div>
                  )}
                </div>
                {isWebBluetoothSupported() && (
                  <p className="hint-message">
                    📶 Not showing up? Some firmware only advertises the Studio
                    Bluetooth service once unlocked — press the unlock key (
                    <code>&amp;studio_unlock</code> behavior) on your keyboard,
                    then try connecting again.
                  </p>
                )}
              </>
            )}
          </section>
        )}
        renderConnected={({ disconnect, deviceName }) => (
          <>
            <section className="card">
              <h2>Device Connection</h2>
              <div className="device-info">
                <h3>✅ Connected to: {deviceName}</h3>
              </div>
              <button className="btn btn-secondary" onClick={disconnect}>
                Disconnect
              </button>
            </section>

            <StudioSection />
          </>
        )}
      />

      <footer className="app-footer">
        <p>
          <strong>Cirque Trackpad — DYA Studio</strong> — DYA Studio tab for the
          cirque-input-module firmware
        </p>
        <p>
          <a
            href={`https://github.com/${GITHUB_REPO}`}
            target="_blank"
            rel="noreferrer"
          >
            {GITHUB_REPO}
          </a>
        </p>
        <p className="template-credit">
          Built from{" "}
          <a
            href={`https://github.com/${TEMPLATE_CREDIT_REPO}`}
            target="_blank"
            rel="noreferrer"
          >
            {TEMPLATE_CREDIT_REPO}
          </a>{" "}
          - AI ready ZMK module template by{" "}
          <a
            href="https://github.com/cormoran"
            target="_blank"
            rel="noreferrer"
          >
            @cormoran
          </a>
        </p>
      </footer>
    </div>
  );
}

/**
 * The 8-section settings editor. It is the SINGLE owner of `useCirqueState()`
 * state: it calls the hook once and prop-drills `value`/`onChange` down to the
 * leaf components, so every control reads/writes the same device state instead
 * of each firing its own `getState` (which would never propagate between
 * siblings).
 */
export function StudioSection() {
  const { state, isConnected, isLoading, error, setField, reset, refresh } =
    useCirqueState();

  if (!isConnected) {
    return (
      <div className="connect-prompt">
        Connect via DYA Studio to edit settings.
      </div>
    );
  }

  if (!state) {
    return (
      <div className="cirque-studio">
        <div className="loading">
          {isLoading ? "Loading…" : "No state received"}
        </div>
        {error && (
          <div className="error" role="alert">
            {error}
          </div>
        )}
      </div>
    );
  }

  const set = setField;

  return (
    <div className="cirque-studio">
      <div className="studio-header">
        <button className="btn btn-secondary" onClick={() => void reset(true)}>
          Reset to defaults
        </button>
        <button className="btn btn-secondary" onClick={() => void refresh()}>
          Refresh
        </button>
      </div>

      {error && (
        <div className="error" role="alert">
          {error}
        </div>
      )}

      <SectionCard title="Mode & Sensitivity">
        <Switch
          label="Relative mode"
          value={state.dataMode === DataMode.DATA_MODE_RELATIVE}
          onChange={(v) =>
            void set(
              "dataMode",
              v ? DataMode.DATA_MODE_RELATIVE : DataMode.DATA_MODE_ABSOLUTE,
              true
            )
          }
        />
        <Switch
          label="2x sensitivity"
          value={state.sensitivity === Sensitivity.SENSITIVITY_2X}
          onChange={(v) =>
            void set(
              "sensitivity",
              v ? Sensitivity.SENSITIVITY_2X : Sensitivity.SENSITIVITY_1X,
              true
            )
          }
        />
      </SectionCard>

      <SectionCard title="Axis">
        <Switch
          label="Invert X"
          value={state.invertX}
          onChange={(v) => void set("invertX", v, true)}
        />
        <Switch
          label="Invert Y"
          value={state.invertY}
          onChange={(v) => void set("invertY", v, true)}
        />
        <Switch
          label="Swap X/Y"
          value={state.swapXy}
          onChange={(v) => void set("swapXy", v, true)}
        />
        <NumberStepper
          label="Rotation"
          value={state.rotateDegrees}
          values={[0, 90, 180, 270]}
          unit="°"
          onChange={(v) => void set("rotateDegrees", v, true)}
        />
      </SectionCard>

      <SectionCard title="Tap">
        <Switch
          label="Primary tap"
          value={state.primaryTapEnable}
          onChange={(v) => void set("primaryTapEnable", v, true)}
        />
        <Switch
          label="Secondary tap (lower-right zone)"
          value={state.secondaryTapEnable}
          onChange={(v) => void set("secondaryTapEnable", v, true)}
        />
        <Switch
          label="Aux tap (upper-left zone)"
          value={state.auxTapEnable}
          onChange={(v) => void set("auxTapEnable", v, true)}
        />
        <Switch
          label="Tap-drag"
          value={state.tapDragEnable}
          onChange={(v) => void set("tapDragEnable", v, true)}
        />
        <Slider
          label="Tap timeout"
          value={state.tapMaxMs}
          min={50}
          max={1000}
          step={10}
          unit=" ms"
          onChange={(v) => void set("tapMaxMs", v, true)}
        />
        <Slider
          label="Tap max movement"
          value={state.tapMaxMovement}
          min={10}
          max={500}
          step={10}
          onChange={(v) => void set("tapMaxMovement", v, true)}
        />
        <Slider
          label="Tap click hold"
          value={state.tapClickMs}
          min={5}
          max={200}
          step={5}
          unit=" ms"
          onChange={(v) => void set("tapClickMs", v, true)}
        />
        <Slider
          label="Drag timeout"
          value={state.tapDragTimeoutMs}
          min={50}
          max={1000}
          step={10}
          unit=" ms"
          onChange={(v) => void set("tapDragTimeoutMs", v, true)}
        />
        <Slider
          label="Drag max movement"
          value={state.tapDragMaxMovement}
          min={10}
          max={500}
          step={10}
          onChange={(v) => void set("tapDragMaxMovement", v, true)}
        />
        <Slider
          label="Secondary zone width"
          value={state.secondaryTapAreaWidth}
          min={0}
          max={500}
          step={10}
          onChange={(v) => void set("secondaryTapAreaWidth", v, true)}
        />
        <Slider
          label="Secondary zone height"
          value={state.secondaryTapAreaHeight}
          min={0}
          max={500}
          step={10}
          onChange={(v) => void set("secondaryTapAreaHeight", v, true)}
        />
        <Slider
          label="Aux zone width"
          value={state.auxTapAreaWidth}
          min={0}
          max={500}
          step={10}
          onChange={(v) => void set("auxTapAreaWidth", v, true)}
        />
        <Slider
          label="Aux zone height"
          value={state.auxTapAreaHeight}
          min={0}
          max={500}
          step={10}
          onChange={(v) => void set("auxTapAreaHeight", v, true)}
        />
      </SectionCard>

      <SectionCard title="Edge Motion">
        <Switch
          label="Enable edge motion"
          value={state.edgeMotionEnable}
          onChange={(v) => void set("edgeMotionEnable", v, true)}
        />
        <Slider
          label="Zone size"
          value={state.edgeMotionZone}
          min={20}
          max={300}
          step={5}
          onChange={(v) => void set("edgeMotionZone", v, true)}
        />
        <Slider
          label="Speed"
          value={state.edgeMotionSpeed}
          min={1}
          max={20}
          step={1}
          onChange={(v) => void set("edgeMotionSpeed", v, true)}
        />
        <Slider
          label="Interval"
          value={state.edgeMotionIntervalMs}
          min={10}
          max={200}
          step={5}
          unit=" ms"
          onChange={(v) => void set("edgeMotionIntervalMs", v, true)}
        />
        <Slider
          label="Start delay"
          value={state.edgeMotionStartMs}
          min={50}
          max={1000}
          step={10}
          unit=" ms"
          onChange={(v) => void set("edgeMotionStartMs", v, true)}
        />
      </SectionCard>

      <SectionCard title="Edge Scroll">
        <Switch
          label="Right edge → vertical scroll"
          value={state.rightEdgeScrollEnable}
          onChange={(v) => void set("rightEdgeScrollEnable", v, true)}
        />
        <Switch
          label="Top edge → horizontal scroll"
          value={state.topEdgeScrollEnable}
          onChange={(v) => void set("topEdgeScrollEnable", v, true)}
        />
        <Switch
          label="Invert scroll"
          value={state.invertScroll}
          onChange={(v) => void set("invertScroll", v, true)}
        />
        <Slider
          label="Scroll zone size"
          value={state.scrollZone}
          min={20}
          max={300}
          step={5}
          onChange={(v) => void set("scrollZone", v, true)}
        />
        <Slider
          label="Scroll divisor"
          value={state.scrollDivisor}
          min={1}
          max={64}
          step={1}
          onChange={(v) => void set("scrollDivisor", v, true)}
        />
      </SectionCard>

      <SectionCard title="Pointer">
        <Slider
          label="Relative multiplier"
          value={state.relativeMultiplier}
          min={1}
          max={20}
          step={1}
          onChange={(v) => void set("relativeMultiplier", v, true)}
        />
        <Slider
          label="Relative divisor"
          value={state.relativeDivisor}
          min={1}
          max={20}
          step={1}
          onChange={(v) => void set("relativeDivisor", v, true)}
        />
        <Slider
          label="Absolute multiplier"
          value={state.absoluteRelativeMultiplier}
          min={1}
          max={20}
          step={1}
          onChange={(v) => void set("absoluteRelativeMultiplier", v, true)}
        />
        <Slider
          label="Absolute divisor"
          value={state.absoluteRelativeDivisor}
          min={1}
          max={20}
          step={1}
          onChange={(v) => void set("absoluteRelativeDivisor", v, true)}
        />
      </SectionCard>

      <SectionCard title="Speed">
        <Slider
          label="Pointer speed position"
          value={state.pointerSpeedPosition}
          min={0}
          max={100}
          step={1}
          onChange={(v) => void set("pointerSpeedPosition", v, true)}
        />
        <Slider
          label="Scroll speed position"
          value={state.scrollSpeedPosition}
          min={0}
          max={100}
          step={1}
          onChange={(v) => void set("scrollSpeedPosition", v, true)}
        />
      </SectionCard>

      <SectionCard title="Misc">
        <Switch
          label="Enable sleep mode"
          value={state.sleepModeEnable}
          onChange={(v) => void set("sleepModeEnable", v, true)}
        />
        <Switch
          label="Enable drag-scroll"
          value={state.dragScrollEnabled}
          onChange={(v) => void set("dragScrollEnabled", v, true)}
        />
      </SectionCard>
    </div>
  );
}

export default App;
