import React, { ButtonHTMLAttributes } from "react";
import { cn } from "@/lib/utils/cn";

interface ButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: "primary" | "secondary" | "outline";
  size?: "sm" | "md" | "lg";
  children: React.ReactNode;
}

export const Button = React.forwardRef<HTMLButtonElement, ButtonProps>(
  ({ className, variant = "primary", size = "md", children, ...props }, ref) => {
    const baseStyles =
      "inline-flex items-center justify-center font-medium rounded-[12px] transition-all duration-300 focus:outline-none focus:ring-2 focus:ring-[#F38B17] disabled:opacity-50 disabled:cursor-not-allowed uppercase tracking-wider";

    const variants = {
      primary:
        "bg-[#F38B17] text-white hover:brightness-110 active:scale-[0.98] shadow-lg shadow-[#F38B17]/20",
      secondary:
        "bg-[#1E4D2B] text-white hover:bg-[#3F7D45] active:scale-[0.98]",
      outline:
        "border border-[#1E4D2B] text-[#F5F5F5] hover:bg-[#1E4D2B]/30 hover:border-[#3F7D45]",
    };

    const sizes = {
      sm: "px-4 py-2 text-sm",
      md: "px-6 py-3 text-base",
      lg: "px-8 py-4 text-lg font-semibold",
    };

    return (
      <button
        ref={ref}
        className={cn(baseStyles, variants[variant], sizes[size], className)}
        {...props}
      >
        {children}
      </button>
    );
  }
);

Button.displayName = "Button";