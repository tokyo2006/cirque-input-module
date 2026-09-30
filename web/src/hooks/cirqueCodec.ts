/**
 * Minimal, dependency-free protobuf encoder/decoder for the Cirque Studio RPC
 * messages declared in `cirqueTypes.ts`.
 *
 * Only the constructs actually used by `cirque.proto` are implemented:
 * varint (bool/uint32/enum) and length-delimited (string + nested message)
 * wire types. This keeps the codec small and deterministic, and avoids a
 * runtime dependency on generated code (see the note in `cirqueTypes.ts`).
 */

import {
  DataMode,
  Sensitivity,
  type CirqueRequest,
  type CirqueResponse,
  type CirqueState,
  type SetStateResponse,
} from "./cirqueTypes";

const WIRE_VARINT = 0;
const WIRE_LENGTH_DELIMITED = 2;

type StateFieldKind = "enum" | "bool" | "uint32";

interface StateFieldDescriptor {
  name: keyof CirqueState;
  num: number;
  kind: StateFieldKind;
}

const STATE_FIELDS: StateFieldDescriptor[] = [
  { name: "dataMode", num: 1, kind: "enum" },
  { name: "sensitivity", num: 2, kind: "enum" },
  { name: "invertX", num: 3, kind: "bool" },
  { name: "invertY", num: 4, kind: "bool" },
  { name: "swapXy", num: 5, kind: "bool" },
  { name: "rotateDegrees", num: 6, kind: "uint32" },
  { name: "primaryTapEnable", num: 10, kind: "bool" },
  { name: "secondaryTapEnable", num: 11, kind: "bool" },
  { name: "auxTapEnable", num: 12, kind: "bool" },
  { name: "tapMaxMs", num: 13, kind: "uint32" },
  { name: "tapMaxMovement", num: 14, kind: "uint32" },
  { name: "tapClickMs", num: 15, kind: "uint32" },
  { name: "tapDragEnable", num: 16, kind: "bool" },
  { name: "tapDragTimeoutMs", num: 17, kind: "uint32" },
  { name: "tapDragMaxMovement", num: 18, kind: "uint32" },
  { name: "secondaryTapAreaWidth", num: 19, kind: "uint32" },
  { name: "secondaryTapAreaHeight", num: 20, kind: "uint32" },
  { name: "auxTapAreaWidth", num: 21, kind: "uint32" },
  { name: "auxTapAreaHeight", num: 22, kind: "uint32" },
  { name: "edgeMotionEnable", num: 30, kind: "bool" },
  { name: "edgeMotionZone", num: 31, kind: "uint32" },
  { name: "edgeMotionSpeed", num: 32, kind: "uint32" },
  { name: "edgeMotionIntervalMs", num: 33, kind: "uint32" },
  { name: "edgeMotionStartMs", num: 34, kind: "uint32" },
  { name: "rightEdgeScrollEnable", num: 40, kind: "bool" },
  { name: "topEdgeScrollEnable", num: 41, kind: "bool" },
  { name: "scrollZone", num: 42, kind: "uint32" },
  { name: "scrollDivisor", num: 43, kind: "uint32" },
  { name: "invertScroll", num: 44, kind: "bool" },
  { name: "relativeMultiplier", num: 50, kind: "uint32" },
  { name: "relativeDivisor", num: 51, kind: "uint32" },
  { name: "absoluteRelativeMultiplier", num: 52, kind: "uint32" },
  { name: "absoluteRelativeDivisor", num: 53, kind: "uint32" },
  { name: "sleepModeEnable", num: 60, kind: "bool" },
  { name: "dragScrollEnabled", num: 80, kind: "bool" },
  { name: "pointerSpeedPosition", num: 90, kind: "uint32" },
  { name: "scrollSpeedPosition", num: 91, kind: "uint32" },
];

const FIELDS_BY_NUM = new Map<number, StateFieldDescriptor>(
  STATE_FIELDS.map((f) => [f.num, f]),
);

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

function encodeVarint(value: number, out: number[]): void {
  let v = value >>> 0;
  while (v >= 0x80) {
    out.push((v & 0x7f) | 0x80);
    v >>>= 7;
  }
  out.push(v);
}

function writeTag(out: number[], fieldNum: number, wireType: number): void {
  encodeVarint((fieldNum << 3) | wireType, out);
}

function writeVarintField(out: number[], fieldNum: number, value: number): void {
  writeTag(out, fieldNum, WIRE_VARINT);
  encodeVarint(value, out);
}

function writeLengthDelimited(
  out: number[],
  fieldNum: number,
  body: number[],
): void {
  writeTag(out, fieldNum, WIRE_LENGTH_DELIMITED);
  encodeVarint(body.length, out);
  for (const b of body) out.push(b);
}

function encodeCirqueState(state: CirqueState): number[] {
  const out: number[] = [];
  for (const field of STATE_FIELDS) {
    const value = state[field.name] as number;
    if (field.kind === "bool") {
      // proto3 default for bool is false; omit falsy values.
      if (value) writeVarintField(out, field.num, 1);
    } else if (value !== 0) {
      writeVarintField(out, field.num, value);
    }
  }
  return out;
}

function encodeSetState(request: {
  state: CirqueState;
  persist: boolean;
}): number[] {
  const out: number[] = [];
  writeLengthDelimited(out, 1, encodeCirqueState(request.state));
  if (request.persist) writeVarintField(out, 2, 1);
  return out;
}

function encodeReset(request: { factoryDefaults: boolean }): number[] {
  const out: number[] = [];
  if (request.factoryDefaults) writeVarintField(out, 1, 1);
  return out;
}

export function encodeRequest(request: CirqueRequest): Uint8Array {
  const out: number[] = [];
  if ("getState" in request) {
    writeLengthDelimited(out, 1, []);
  } else if ("setState" in request) {
    writeLengthDelimited(out, 2, encodeSetState(request.setState));
  } else if ("reset" in request) {
    writeLengthDelimited(out, 3, encodeReset(request.reset));
  }
  return new Uint8Array(out);
}

// ---------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------

interface VarintResult {
  value: number;
  pos: number;
}

function readVarint(bytes: Uint8Array, pos: number): VarintResult {
  let value = 0;
  let shift = 0;
  for (;;) {
    if (pos >= bytes.length) throw new Error("malformed varint");
    const b = bytes[pos++];
    value |= (b & 0x7f) << shift;
    if ((b & 0x80) === 0) break;
    shift += 7;
    if (shift > 35) throw new Error("varint too long");
  }
  return { value: value >>> 0, pos };
}

interface TagResult {
  fieldNum: number;
  wireType: number;
  pos: number;
}

function readTag(bytes: Uint8Array, pos: number): TagResult {
  const { value, pos: next } = readVarint(bytes, pos);
  return { fieldNum: value >>> 3, wireType: value & 0x7, pos: next };
}

function skipField(bytes: Uint8Array, pos: number, wireType: number): number {
  switch (wireType) {
    case WIRE_VARINT:
      return readVarint(bytes, pos).pos;
    case WIRE_LENGTH_DELIMITED: {
      const { value, pos: next } = readVarint(bytes, pos);
      return next + value;
    }
    case 1: // 64-bit
      return pos + 8;
    case 5: // 32-bit
      return pos + 4;
    default:
      throw new Error(`unsupported wire type: ${wireType}`);
  }
}

export function defaultCirqueState(): CirqueState {
  return {
    dataMode: DataMode.DATA_MODE_ABSOLUTE,
    sensitivity: Sensitivity.SENSITIVITY_1X,
    invertX: false,
    invertY: false,
    swapXy: false,
    rotateDegrees: 0,
    primaryTapEnable: false,
    secondaryTapEnable: false,
    auxTapEnable: false,
    tapMaxMs: 0,
    tapMaxMovement: 0,
    tapClickMs: 0,
    tapDragEnable: false,
    tapDragTimeoutMs: 0,
    tapDragMaxMovement: 0,
    secondaryTapAreaWidth: 0,
    secondaryTapAreaHeight: 0,
    auxTapAreaWidth: 0,
    auxTapAreaHeight: 0,
    edgeMotionEnable: false,
    edgeMotionZone: 0,
    edgeMotionSpeed: 0,
    edgeMotionIntervalMs: 0,
    edgeMotionStartMs: 0,
    rightEdgeScrollEnable: false,
    topEdgeScrollEnable: false,
    scrollZone: 0,
    scrollDivisor: 0,
    invertScroll: false,
    relativeMultiplier: 0,
    relativeDivisor: 0,
    absoluteRelativeMultiplier: 0,
    absoluteRelativeDivisor: 0,
    sleepModeEnable: false,
    dragScrollEnabled: false,
    pointerSpeedPosition: 0,
    scrollSpeedPosition: 0,
  };
}

function decodeString(bytes: Uint8Array): string {
  return new TextDecoder().decode(bytes);
}

function decodeCirqueState(bytes: Uint8Array): CirqueState {
  const state = defaultCirqueState();
  let pos = 0;
  while (pos < bytes.length) {
    const tag = readTag(bytes, pos);
    pos = tag.pos;
    const field = FIELDS_BY_NUM.get(tag.fieldNum);
    if (!field || tag.wireType !== WIRE_VARINT) {
      pos = skipField(bytes, pos, tag.wireType);
      continue;
    }
    const { value, pos: next } = readVarint(bytes, pos);
    pos = next;
    const record = state as unknown as Record<string, unknown>;
    if (field.kind === "bool") {
      record[field.name] = value !== 0;
    } else {
      record[field.name] = value;
    }
  }
  return state;
}

function decodeSetStateResponse(bytes: Uint8Array): SetStateResponse {
  let state = defaultCirqueState();
  let persisted = false;
  let pos = 0;
  while (pos < bytes.length) {
    const tag = readTag(bytes, pos);
    pos = tag.pos;
    if (tag.fieldNum === 1 && tag.wireType === WIRE_LENGTH_DELIMITED) {
      const { value, pos: next } = readVarint(bytes, pos);
      state = decodeCirqueState(bytes.subarray(next, next + value));
      pos = next + value;
    } else if (tag.fieldNum === 2 && tag.wireType === WIRE_VARINT) {
      const { value, pos: next } = readVarint(bytes, pos);
      persisted = value !== 0;
      pos = next;
    } else {
      pos = skipField(bytes, pos, tag.wireType);
    }
  }
  return { state, persisted };
}

export function decodeResponse(bytes: Uint8Array): CirqueResponse {
  let pos = 0;
  let response: CirqueResponse = { error: { message: "empty response" } };
  while (pos < bytes.length) {
    const tag = readTag(bytes, pos);
    pos = tag.pos;
    if (tag.wireType !== WIRE_LENGTH_DELIMITED) {
      pos = skipField(bytes, pos, tag.wireType);
      continue;
    }
    const { value, pos: next } = readVarint(bytes, pos);
    const body = bytes.subarray(next, next + value);
    pos = next + value;
    switch (tag.fieldNum) {
      case 1:
        response = { error: { message: decodeString(body) } };
        break;
      case 2:
        response = { getState: { state: decodeCirqueState(body) } };
        break;
      case 3:
        response = { setState: decodeSetStateResponse(body) };
        break;
      case 4:
        response = { reset: { state: decodeCirqueState(body) } };
        break;
      default:
        break;
    }
  }
  return response;
}
