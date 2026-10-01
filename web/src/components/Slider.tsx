import { useEffect, useState } from "react";
import { useCirqueState } from "../hooks/useCirqueState";
import type { CirqueState } from "../hooks/cirqueTypes";

type NumericField = {
  [K in keyof CirqueState]: CirqueState[K] extends number ? K : never;
}[keyof CirqueState];

interface SliderProps {
  field: NumericField;
  label: string;
  min: number;
  max: number;
  step?: number;
  unit?: string;
}

export function Slider({
  field,
  label,
  min,
  max,
  step = 1,
  unit,
}: SliderProps) {
  const { state, setField } = useCirqueState();
  const [local, setLocal] = useState<number | null>(null);

  const value = local ?? state?.[field] ?? min;

  useEffect(() => {
    if (local === null) return;
    const id = setTimeout(() => {
      void setField(field, local, true);
      setLocal(null);
    }, 250);
    return () => clearTimeout(id);
  }, [local, field, setField]);

  return (
    <div className="slider">
      <label>
        {label}: {value}
        {unit}
      </label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => setLocal(Number(e.target.value))}
      />
    </div>
  );
}
