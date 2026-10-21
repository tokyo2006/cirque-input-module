/**
 * Inline TypeScript types mirroring `proto/tokyo2006/cirque/cirque.proto`.
 *
 * These are hand-maintained (not generated) because the `buf generate` /
 * ts-proto codegen path is not currently wired up in this scaffold:
 *   - the proto references the `(nanopb).max_size` extension without importing
 *     `nanopb.proto`, so `buf generate` fails to resolve it;
 *   - `protoc-gen-ts_proto` is not present on PATH for the `local:` plugin.
 *
 * Field names follow ts-proto's default snake_case -> camelCase convention so
 * the hook surface matches what generated code would have produced.
 */

export const DataMode = {
  DATA_MODE_ABSOLUTE: 0,
  DATA_MODE_RELATIVE: 1,
} as const;
export type DataMode = (typeof DataMode)[keyof typeof DataMode];

export const Sensitivity = {
  SENSITIVITY_1X: 0,
  SENSITIVITY_2X: 1,
} as const;
export type Sensitivity = (typeof Sensitivity)[keyof typeof Sensitivity];

export interface CirqueState {
  dataMode: DataMode;
  sensitivity: Sensitivity;

  invertX: boolean;
  invertY: boolean;
  swapXy: boolean;
  rotateDegrees: number;

  primaryTapEnable: boolean;
  secondaryTapEnable: boolean;
  auxTapEnable: boolean;
  tapMaxMs: number;
  tapMaxMovement: number;
  tapClickMs: number;
  tapDragEnable: boolean;
  tapDragTimeoutMs: number;
  tapDragMaxMovement: number;
  secondaryTapAreaWidth: number;
  secondaryTapAreaHeight: number;
  auxTapAreaWidth: number;
  auxTapAreaHeight: number;

  edgeMotionEnable: boolean;
  edgeMotionZone: number;
  edgeMotionSpeed: number;
  edgeMotionIntervalMs: number;
  edgeMotionStartMs: number;

  rightEdgeScrollEnable: boolean;
  topEdgeScrollEnable: boolean;
  scrollZone: number;
  scrollDivisor: number;
  invertScroll: boolean;

  relativeMultiplier: number;
  relativeDivisor: number;
  absoluteRelativeMultiplier: number;
  absoluteRelativeDivisor: number;

  sleepModeEnable: boolean;
  dragScrollEnabled: boolean;
  pointerSpeedPosition: number;
  scrollSpeedPosition: number;
}

// eslint-disable-next-line @typescript-eslint/no-empty-object-type -- empty proto message
export interface GetStateRequest {}

export interface GetStateResponse {
  state: CirqueState;
}

export interface SetStateRequest {
  state: CirqueState;
  persist: boolean;
}

export interface SetStateResponse {
  state: CirqueState;
  persisted: boolean;
}

export interface ResetRequest {
  factoryDefaults: boolean;
}

export interface ResetResponse {
  state: CirqueState;
}

export interface ErrorResponse {
  message: string;
}

export type CirqueRequest =
  | { getState: GetStateRequest }
  | { setState: SetStateRequest }
  | { reset: ResetRequest };

export type CirqueResponse =
  | { error: ErrorResponse }
  | { getState: GetStateResponse }
  | { setState: SetStateResponse }
  | { reset: ResetResponse };
