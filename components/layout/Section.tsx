import React, { HTMLAttributes } from "react";
import { cn } from "@/lib/utils/cn";

interface SectionProps extends HTMLAttributes<HTMLElement> {
  id?: string;
  children: React.ReactNode;
}

export const Section = ({ id, className, children, ...props }: SectionProps) => {
  return (
    <section
      id={id}
      className={cn("py-20 md:py-32 relative overflow-hidden", className)}
      {...props}
    >
      {children}
    </section>
  );
};