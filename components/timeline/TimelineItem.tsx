import React, { ReactNode } from "react";
import { Card } from "@/components/ui/Card";
import { Badge } from "@/components/ui/Badge";

interface TimelineItemProps {
  step: string;
  title: string;
  description: string;
  icon: ReactNode;
  isLast?: boolean;
}

export const TimelineItem = ({
  step,
  title,
  description,
  icon,
  isLast = false,
}: TimelineItemProps) => {
  return (
    <div className="relative flex items-start gap-6 md:gap-10 pb-12 group">
      {/* Linha vertical condutora */}
      {!isLast && (
        <div className="absolute left-[23px] md:left-[27px] top-[48px] bottom-0 w-[2px] bg-[#1E4D2B]/50 group-hover:bg-[#3F7D45] transition-colors duration-300" />
      )}

      {/* Marcador da Etapa (Dot / Ícone) */}
      <div className="relative z-10 flex items-center justify-center w-12 h-12 md:w-14 md:h-14 rounded-full bg-[#132416] border-2 border-[#1E4D2B] text-[#F38B17] group-hover:border-[#F38B17] group-hover:scale-110 transition-all duration-300 shrink-0 shadow-lg shadow-[#08110B]">
        {icon}
      </div>

      {/* Conteúdo da Etapa */}
      <div className="flex-1 pt-1">
        <Card className="bg-[#132416]/70 border-[#1E4D2B]/40 hover:border-[#3F7D45] transition-all duration-300 p-6">
          <div className="flex items-center gap-3 mb-2">
            <Badge variant="orange" className="text-[10px]">
              Etapa {step}
            </Badge>
          </div>
          <h3 className="text-2xl font-bold text-[#F5F5F5] uppercase mb-2">
            {title}
          </h3>
          <p className="text-[#A5A5A5] text-base leading-relaxed">
            {description}
          </p>
        </Card>
      </div>
    </div>
  );
};