import { useEffect, useState } from "react";

interface SliderProps {
  label: string;
  value: number;
  onChange: (value: number) => void;
  min: number;
  max: number;
  step?: number;
  unit?: string;
}

export function Slider({
  label,
  value,
  onChange,
  min,
  max,
  step = 1,
  unit,
}: SliderProps) {
  const [local, setLocal] = useState<number | null>(null);

  const display = local ?? value;

  useEffect(() => {
    if (local === null) return;
    const id = setTimeout(() => {
      onChange(local);
      setLocal(null);
    }, 250);
    return () => clearTimeout(id);
  }, [local, onChange]);

  return (
    <div className="slider">
      <label>
        {label}: {display}
        {unit}
      </label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={display}
        onChange={(e) => setLocal(Number(e.target.value))}
      />
    </div>
  );
}
