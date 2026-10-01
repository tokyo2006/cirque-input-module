interface NumberStepperProps {
  label: string;
  value: number;
  values: number[];
  onChange: (value: number) => void;
  unit?: string;
}

export function NumberStepper({
  label,
  value,
  values,
  onChange,
  unit,
}: NumberStepperProps) {
  const index = Math.max(0, values.indexOf(value));

  const stepTo = (direction: 1 | -1) => {
    const next = values[(index + direction + values.length) % values.length];
    onChange(next);
  };

  return (
    <div className="number-stepper">
      <label>
        {label}: {value}
        {unit}
      </label>
      <div className="number-stepper__controls">
        <button type="button" onClick={() => stepTo(-1)}>
          −
        </button>
        <span>{value}</span>
        <button type="button" onClick={() => stepTo(1)}>
          +
        </button>
      </div>
    </div>
  );
}
