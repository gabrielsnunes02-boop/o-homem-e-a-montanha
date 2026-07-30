"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { Card } from "@/components/ui/Card";
import { Flame, Shield, Footprints } from "lucide-react";

export const AboutSection = () => {
  return (
    <section id="sobre" className="py-24 bg-[#0D1B11] border-t border-b border-[#1E4D2B]/30 relative">
      <Container>
        <div className="max-w-3xl mx-auto text-center mb-16">
          <span className="text-[#F38B17] text-xs font-bold uppercase tracking-widest block mb-3">
            O Propósito da Jornada
          </span>
          <h2 className="text-4xl sm:text-5xl md:text-6xl font-extrabold text-[#F5F5F5] tracking-tight uppercase mb-6">
            Todo Homem Enfrenta <span className="text-[#3F7D45]">Montanhas</span>
          </h2>
          <p className="text-lg sm:text-xl text-[#A5A5A5] leading-relaxed">
            Existem momentos na vida em que a rotina, o barulho e o peso das responsabilidades mascaram o que realmente importa. A montanha não é apenas um lugar físico — é o ambiente onde você é testado, confrontado e renovado.
          </p>
        </div>

        {/* Cards dos Pilares da Jornada */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-8">
          <Card className="bg-[#132416]/80 border-[#1E4D2B]/50 hover:border-[#3F7D45] transition-all duration-300">
            <div className="p-3 bg-[#1E4D2B]/30 rounded-xl w-fit mb-6 text-[#F38B17]">
              <Footprints className="w-8 h-8" />
            </div>
            <h3 className="text-2xl font-bold text-[#F5F5F5] mb-3 uppercase">
              A Subida
            </h3>
            <p className="text-[#A5A5A5] text-base leading-relaxed">
              Uma caminhada noturna que exige esforço, perseverança e superação. Cada passo representa o abandono do comodismo rumo ao topo.
            </p>
          </Card>

          <Card className="bg-[#132416]/80 border-[#1E4D2B]/50 hover:border-[#3F7D45] transition-all duration-300">
            <div className="p-3 bg-[#1E4D2B]/30 rounded-xl w-fit mb-6 text-[#3F7D45]">
              <Shield className="w-8 h-8" />
            </div>
            <h3 className="text-2xl font-bold text-[#F5F5F5] mb-3 uppercase">
              O Silêncio
            </h3>
            <p className="text-[#A5A5A5] text-base leading-relaxed">
              Longe da cidade e das distrações. Paradas estratégicas ao longo da trilha para ministrações profundas, reflexão e alinhamento espiritual.
            </p>
          </Card>

          <Card className="bg-[#132416]/80 border-[#1E4D2B]/50 hover:border-[#F38B17] transition-all duration-300">
            <div className="p-3 bg-[#1E4D2B]/30 rounded-xl w-fit mb-6 text-[#F38B17]">
              <Flame className="w-8 h-8" />
            </div>
            <h3 className="text-2xl font-bold text-[#F5F5F5] mb-3 uppercase">
              O Topo
            </h3>
            <p className="text-[#A5A5A5] text-base leading-relaxed">
              O momento do encontro. Oração, adoração intensa e comunhão no ponto mais alto, marcando uma decisão inegociável de transformação.
            </p>
          </Card>
        </div>

        {/* Mensagem de Citação Final */}
        <div className="mt-16 p-8 bg-[#132416] border border-[#1E4D2B]/40 rounded-2xl text-center max-w-4xl mx-auto">
          <p className="text-xl sm:text-2xl font-semibold text-[#F5F5F5] italic">
            &ldquo;A montanha não muda Deus. Ela muda você.&rdquo;
          </p>
        </div>
      </Container>
    </section>
  );
};