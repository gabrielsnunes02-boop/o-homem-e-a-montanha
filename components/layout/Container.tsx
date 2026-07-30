import React, { HTMLAttributes } from "react";
import { cn } from "@/lib/utils/cn";

interface ContainerProps extends HTMLAttributes<HTMLDivElement> {
  children: React.ReactNode;
}

export const Container = ({ className, children, ...props }: ContainerProps) => {
  return (
    <div
      className={cn("max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 w-full", className)}
      {...props}
    >
      {children}
    </div>
  );
};