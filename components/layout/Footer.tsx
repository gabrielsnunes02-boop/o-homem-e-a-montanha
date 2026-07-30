"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { Mountain, ShieldCheck } from "lucide-react";
import { scrollToSection } from "@/lib/utils/scroll";

export const Footer = () => {
  return (
    <footer className="bg-[#08110B] border-t border-[#1E4D2B]/40 py-12 text-[#A5A5A5] text-sm">
      <Container>
        <div className="flex flex-col md:flex-row items-center justify-between gap-6 pb-8 border-b border-[#1E4D2B]/30">
          {/* Logo e Nome */}
          <div className="flex items-center gap-3">
            <div className="p-2 bg-[#132416] border border-[#1E4D2B] rounded-xl text-[#F38B17]">
              <Mountain className="w-6 h-6" />
            </div>
            <div>
              <span className="block font-bold text-[#F5F5F5] uppercase tracking-wider">
                O Homem e a Montanha
              </span>
              <span className="text-xs text-[#3F7D45]">
                Igreja Viva em Missões
              </span>
            </div>
          </div>

          {/* Links Rápidos */}
          <nav className="flex flex-wrap justify-center gap-6 text-xs uppercase tracking-widest font-semibold">
            <button
              onClick={() => scrollToSection("sobre")}
              className="hover:text-[#F38B17] transition-colors cursor-pointer"
            >
              Sobre
            </button>
            <button
              onClick={() => scrollToSection("jornada")}
              className="hover:text-[#F38B17] transition-colors cursor-pointer"
            >
              A Jornada
            </button>
            <button
              onClick={() => scrollToSection("inscricao")}
              className="hover:text-[#F38B17] transition-colors cursor-pointer"
            >
              Inscrição
            </button>
            <button
              onClick={() => scrollToSection("faq")}
              className="hover:text-[#F38B17] transition-colors cursor-pointer"
            >
              FAQ
            </button>
          </nav>
        </div>

        {/* Direitos Reservados e Aviso de Segurança */}
        <div className="pt-8 flex flex-col sm:flex-row items-center justify-between gap-4 text-xs text-center sm:text-left">
          <p>
            &copy; {new Date().getFullYear()} O Homem e a Montanha. Todos os direitos reservados.
          </p>
          <div className="flex items-center gap-2 text-[#3F7D45]">
            <ShieldCheck className="w-4 h-4 text-[#F38B17]" />
            <span>Ambiente Seguro & Expedição Monitorada</span>
          </div>
        </div>
      </Container>
    </footer>
  );
};