import React, { InputHTMLAttributes } from "react";
import { cn } from "@/lib/utils/cn";

interface InputProps extends InputHTMLAttributes<HTMLInputElement> {
  label?: string;
  error?: string;
}

export const Input = React.forwardRef<HTMLInputElement, InputProps>(
  ({ className, label, error, type = "text", ...props }, ref) => {
    return (
      <div className="w-full flex flex-col gap-2">
        {label && (
          <label className="text-sm font-medium text-[#A5A5A5] tracking-wide">
            {label}
          </label>
        )}
        <input
          type={type}
          ref={ref}
          className={cn(
            "w-full bg-[#132416] border border-[#1E4D2B]/60 rounded-[16px] px-4 py-3 text-[#F5F5F5] placeholder-[#A5A5A5]/60 focus:outline-none focus:border-[#F38B17] focus:ring-1 focus:ring-[#F38B17] transition-all duration-200",
            error && "border-red-500 focus:border-red-500 focus:ring-red-500",
            className
          )}
          {...props}
        />
        {error && <span className="text-xs text-red-400 mt-1">{error}</span>}
      </div>
    );
  }
);

Input.displayName = "Input";