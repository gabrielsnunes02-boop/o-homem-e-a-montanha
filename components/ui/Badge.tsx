import React, { HTMLAttributes } from "react";
import { cn } from "@/lib/utils/cn";

interface BadgeProps extends HTMLAttributes<HTMLSpanElement> {
  children: React.ReactNode;
  variant?: "orange" | "green" | "gray";
}

export const Badge = ({
  className,
  children,
  variant = "orange",
  ...props
}: BadgeProps) => {
  const variants = {
    orange: "bg-[#F38B17]/15 text-[#F38B17] border-[#F38B17]/30",
    green: "bg-[#1E4D2B]/40 text-[#3F7D45] border-[#3F7D45]/40",
    gray: "bg-[#132416] text-[#A5A5A5] border-[#A5A5A5]/20",
  };

  return (
    <span
      className={cn(
        "inline-flex items-center px-3 py-1 rounded-full text-xs font-semibold uppercase tracking-wider border",
        variants[variant],
        className
      )}
      {...props}
    >
      {children}
    </span>
  );
};