import React, { HTMLAttributes } from "react";
import { cn } from "@/lib/utils/cn";

interface CardProps extends HTMLAttributes<HTMLDivElement> {
  children: React.ReactNode;
}

export const Card = ({ className, children, ...props }: CardProps) => {
  return (
    <div
      className={cn(
        "bg-[#132416] border border-[#1E4D2B]/40 rounded-[16px] p-6 shadow-xl transition-all duration-300 hover:border-[#1E4D2B]",
        className
      )}
      {...props}
    >
      {children}
    </div>
  );
};