import { useCirqueState } from "../hooks/useCirqueState";
import type { CirqueState } from "../hooks/cirqueTypes";

type BooleanField = {
  [K in keyof CirqueState]: CirqueState[K] extends boolean ? K : never;
}[keyof CirqueState];

interface SwitchProps {
  field: BooleanField;
  label: string;
}

export function Switch({ field, label }: SwitchProps) {
  const { state, setField } = useCirqueState();
  const value = state?.[field] ?? false;

  return (
    <div className="switch">
      <label>
        <input
          type="checkbox"
          checked={value}
          onChange={(e) => void setField(field, e.target.checked, true)}
        />
        <span>{label}</span>
      </label>
    </div>
  );
}
