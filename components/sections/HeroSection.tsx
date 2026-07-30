"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { Button } from "@/components/ui/Button";
import { Badge } from "@/components/ui/Badge";
import { Mountain, Compass, ShieldAlert, ArrowDown } from "lucide-react";
import { scrollToSection } from "@/lib/utils/scroll";

export const HeroSection = () => {
  return (
    <section className="relative min-h-screen flex items-center justify-center pt-24 pb-16 overflow-hidden bg-gradient-to-b from-[#08110B] via-[#0D1B11] to-[#08110B]">
      {/* Elemento de Brilho / Overlay Sutil de Fundo */}
      <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_top,_var(--tw-gradient-stops))] from-[#1E4D2B]/20 via-[#08110B]/80 to-[#08110B] pointer-events-none" />

      {/* Grid de Textura Discreta */}
      <div className="absolute inset-0 bg-[linear-gradient(to_right,#1E4D2B10_1px,transparent_1px),linear-gradient(to_bottom,#1E4D2B10_1px,transparent_1px)] bg-[size:4rem_4rem] [mask-image:radial-gradient(ellipse_60%_50%_at_50%_0%,#000_70%,transparent_100%)] pointer-events-none" />

      <Container className="relative z-10 flex flex-col items-center text-center">
        {/* Badge da Organização */}
        <div className="mb-6 animate-fade-in">
          <Badge variant="green" className="gap-2 py-1.5 px-4 text-xs tracking-widest">
            <Mountain className="w-3.5 h-3.5 text-[#F38B17]" />
            Igreja Viva em Missões Apresenta
          </Badge>
        </div>

        {/* Título Principal de Impacto */}
        <h1 className="text-5xl sm:text-7xl md:text-8xl font-extrabold text-[#F5F5F5] tracking-tight leading-none mb-4 max-w-5xl">
          O HOMEM E A <span className="text-transparent bg-clip-text bg-gradient-to-r from-[#3F7D45] via-[#F38B17] to-[#F38B17]">MONTANHA</span>
        </h1>

        {/* Subtítulo / Tema */}
        <p className="text-xl sm:text-2xl md:text-3xl text-[#F38B17] font-medium tracking-wide uppercase mb-6">
          O desafio que te transforma.
        </p>

        {/* Citação do Versículo Base */}
        <blockquote className="max-w-2xl text-[#A5A5A5] text-base sm:text-lg italic font-normal mb-10 leading-relaxed border-l-2 border-[#1E4D2B] pl-4 sm:pl-0 sm:border-l-0">
          &ldquo;Seja forte e corajoso. Não se apavore nem desanime, pois o Senhor, o seu Deus, estará com você por onde você andar.&rdquo;
          <footer className="text-xs text-[#3F7D45] font-semibold mt-2 not-italic tracking-wider uppercase">
            — Josué 1:9
          </footer>
        </blockquote>

        {/* Chamada para Ação (CTA) */}
        <div className="flex flex-col sm:flex-row gap-4 w-full sm:w-auto mb-16">
          <Button
            size="lg"
            onClick={() => scrollToSection("inscricao")}
            className="w-full sm:w-auto text-base sm:text-lg py-4 px-10 font-bold shadow-xl shadow-[#F38B17]/20 cursor-pointer"
          >
            ACEITAR O CHAMADO
          </Button>
          
          <Button
            variant="outline"
            size="lg"
            onClick={() => scrollToSection("jornada")}
            className="w-full sm:w-auto text-base sm:text-lg py-4 px-8 cursor-pointer"
          >
            CONHECER A JORNADA
          </Button>
        </div>

        {/* Cards Informativos Rápidos */}
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 w-full max-w-3xl text-left border-t border-[#1E4D2B]/30 pt-8">
          <div className="bg-[#132416]/60 border border-[#1E4D2B]/40 rounded-xl p-4 flex items-center gap-3">
            <Compass className="w-6 h-6 text-[#F38B17] shrink-0" />
            <div>
              <span className="block text-xs text-[#A5A5A5] uppercase">Formato</span>
              <span className="text-sm font-semibold text-[#F5F5F5]">Expedição Noturna</span>
            </div>
          </div>

          <div className="bg-[#132416]/60 border border-[#1E4D2B]/40 rounded-xl p-4 flex items-center gap-3">
            <ShieldAlert className="w-6 h-6 text-[#3F7D45] shrink-0" />
            <div>
              <span className="block text-xs text-[#A5A5A5] uppercase">Público</span>
              <span className="text-sm font-semibold text-[#F5F5F5]">Exclusivo para Homens</span>
            </div>
          </div>

          <div className="bg-[#132416]/60 border border-[#1E4D2B]/40 rounded-xl p-4 flex items-center gap-3">
            <Mountain className="w-6 h-6 text-[#F38B17] shrink-0" />
            <div>
              <span className="block text-xs text-[#A5A5A5] uppercase">Experiência</span>
              <span className="text-sm font-semibold text-[#F5F5F5]">Subida & Ministrações</span>
            </div>
          </div>
        </div>

        {/* Indicador de Rolagem */}
        <div className="mt-12 text-[#A5A5A5] animate-bounce cursor-pointer" onClick={() => scrollToSection("sobre")}>
          <ArrowDown className="w-5 h-5 opacity-60 hover:opacity-100 transition-opacity" />
        </div>
      </Container>
    </section>
  );
};