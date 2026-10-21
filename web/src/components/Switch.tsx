interface SwitchProps {
  label: string;
  value: boolean;
  onChange: (value: boolean) => void;
}

export function Switch({ label, value, onChange }: SwitchProps) {
  return (
    <div className="switch">
      <label>
        <input
          type="checkbox"
          checked={value}
          onChange={(e) => onChange(e.target.checked)}
        />
        <span>{label}</span>
      </label>
    </div>
  );
}
