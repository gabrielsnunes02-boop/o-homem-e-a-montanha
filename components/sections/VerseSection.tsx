"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { Mountain } from "lucide-react";

export const VerseSection = () => {
  return (
    <section className="py-28 bg-[#0D1B11] border-y border-[#1E4D2B]/40 relative overflow-hidden">
      {/* Luz ambiente de destaque */}
      <div className="absolute inset-0 bg-[radial-gradient(circle_at_center,_var(--tw-gradient-stops))] from-[#1E4D2B]/30 via-transparent to-transparent pointer-events-none" />

      <Container className="relative z-10 text-center max-w-4xl">
        <Mountain className="w-12 h-12 text-[#F38B17] mx-auto mb-8 opacity-80" />

        <h2 className="text-3xl sm:text-4xl md:text-5xl font-extrabold text-[#F5F5F5] uppercase tracking-wide leading-snug mb-8">
          &ldquo;Não fui eu que lhe ordenei? Seja forte e corajoso. Não se apavore nem desanime, pois o Senhor, o seu Deus, estará com você por onde você andar.&rdquo;
        </h2>

        <span className="inline-block text-[#F38B17] font-bold text-lg tracking-widest uppercase border-b-2 border-[#F38B17] pb-1">
          Josué 1:9
        </span>
      </Container>
    </section>
  );
};