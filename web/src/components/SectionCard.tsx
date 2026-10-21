import { useState, type ReactNode } from "react";

interface SectionCardProps {
  title: string;
  children: ReactNode;
  defaultOpen?: boolean;
}

export function SectionCard({
  title,
  children,
  defaultOpen = true,
}: SectionCardProps) {
  const [open, setOpen] = useState(defaultOpen);

  return (
    <section className="section-card">
      <button
        type="button"
        className="section-card__header"
        aria-expanded={open}
        onClick={() => setOpen((prev) => !prev)}
      >
        <span className="section-card__title">{title}</span>
        <span className="section-card__toggle" aria-hidden="true">
          {open ? "−" : "+"}
        </span>
      </button>
      {open && <div className="section-card__body">{children}</div>}
    </section>
  );
}
