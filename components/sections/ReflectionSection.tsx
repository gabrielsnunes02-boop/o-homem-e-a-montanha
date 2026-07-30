"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { Button } from "@/components/ui/Button";
import { scrollToSection } from "@/lib/utils/scroll";
import { Flame } from "lucide-react";

export const ReflectionSection = () => {
  return (
    <section className="py-20 bg-[#0D1B11] relative">
      <Container>
        <div className="bg-gradient-to-r from-[#132416] via-[#1E4D2B]/60 to-[#132416] border border-[#1E4D2B] rounded-3xl p-8 sm:p-12 text-center max-w-4xl mx-auto shadow-2xl relative overflow-hidden">
          <div className="absolute -right-10 -bottom-10 opacity-10 pointer-events-none">
            <Flame className="w-64 h-64 text-[#F38B17]" />
          </div>

          <span className="text-[#F38B17] text-xs font-bold uppercase tracking-widest block mb-4">
            A Hora da Decisão
          </span>

          <h2 className="text-3xl sm:text-4xl font-extrabold text-[#F5F5F5] uppercase tracking-tight mb-6">
            Você vai continuar no mesmo lugar ou vai encarar a montanha?
          </h2>

          <p className="text-[#A5A5A5] text-base sm:text-lg mb-8 max-w-2xl mx-auto leading-relaxed">
            As vagas são limitadas para garantir a segurança e o alinhamento da expedição. Não deixe para depois a decisão que pode mudar a sua trajetória.
          </p>

          <Button
            size="lg"
            onClick={() => scrollToSection("inscricao")}
            className="font-bold py-4 px-10 cursor-pointer"
          >
            GARANTIR MINHA VAGA AGORA
          </Button>
        </div>
      </Container>
    </section>
  );
};