import { useCirqueState } from "../hooks/useCirqueState";
import type { CirqueState } from "../hooks/cirqueTypes";

type NumericField = {
  [K in keyof CirqueState]: CirqueState[K] extends number ? K : never;
}[keyof CirqueState];

interface NumberStepperProps {
  field: NumericField;
  label: string;
  values: number[];
  unit?: string;
}

export function NumberStepper({
  field,
  label,
  values,
  unit,
}: NumberStepperProps) {
  const { state, setField } = useCirqueState();
  const current = state?.[field] ?? values[0];
  const index = Math.max(0, values.indexOf(current));

  const stepTo = (direction: 1 | -1) => {
    const next = values[(index + direction + values.length) % values.length];
    void setField(field, next, true);
  };

  return (
    <div className="number-stepper">
      <label>
        {label}: {current}
        {unit}
      </label>
      <div className="number-stepper__controls">
        <button type="button" onClick={() => stepTo(-1)}>
          −
        </button>
        <span>{current}</span>
        <button type="button" onClick={() => stepTo(1)}>
          +
        </button>
      </div>
    </div>
  );
}
