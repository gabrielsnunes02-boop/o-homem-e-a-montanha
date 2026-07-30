import React from "react";
import { cn } from "@/lib/utils/cn";

interface ProgressProps {
  value: number; // 0 a 100
  className?: string;
}

export const Progress = ({ value, className }: ProgressProps) => {
  const clampedValue = Math.min(100, Math.max(0, value));

  return (
    <div
      className={cn(
        "w-full bg-[#132416] border border-[#1E4D2B]/50 rounded-full h-3 overflow-hidden p-[2px]",
        className
      )}
    >
      <div
        className="bg-gradient-to-r from-[#1E4D2B] via-[#3F7D45] to-[#F38B17] h-full rounded-full transition-all duration-500 ease-out"
        style={{ width: `${clampedValue}%` }}
      />
    </div>
  );
};