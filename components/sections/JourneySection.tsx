"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { TimelineItem } from "@/components/timeline/TimelineItem";
import {
  Church,
  Compass,
  Footprints,
  VolumeX,
  ShieldCheck,
  Mountain,
  Send,
} from "lucide-react";

export const JourneySection = () => {
  const steps = [
    {
      step: "01",
      title: "A Base",
      description:
        "Concentração na igreja. Recepção, alinhamento dos grupos, orientações de segurança e check-in dos participantes.",
      icon: <Church className="w-5 h-5 md:w-6 md:h-6" />,
    },
    {
      step: "02",
      title: "A Convocação",
      description:
        "Briefing final e deslocamento conjunto em direção ao ponto de partida da subida da montanha.",
      icon: <Compass className="w-5 h-5 md:w-6 md:h-6" />,
    },
    {
      step: "03",
      title: "O Primeiro Passo",
      description:
        "Início da caminhada na trilha. O momento em que o conforto fica para trás e a expedição começa de fato.",
      icon: <Footprints className="w-5 h-5 md:w-6 md:h-6" />,
    },
    {
      step: "04",
      title: "O Silêncio da Subida",
      description:
        "Trecho em silêncio contemplativo. Desconectar dos ruídos do cotidiano para ouvir a voz de Deus no ambiente da criação.",
      icon: <VolumeX className="w-5 h-5 md:w-6 md:h-6" />,
    },
    {
      step: "05",
      title: "A Coragem",
      description:
        "Paradas estratégicas ao longo da caminhada para ministrações, confrontos de vida e fortalecimento mútuo.",
      icon: <ShieldCheck className="w-5 h-5 md:w-6 md:h-6" />,
    },
    {
      step: "06",
      title: "O Topo",
      description:
        "O ponto mais alto da expedição. Ministração principal, oração intensa, adoração e comunhão ao amanhecer/noite.",
      icon: <Mountain className="w-5 h-5 md:w-6 md:h-6" />,
    },
    {
      step: "07",
      title: "O Envio",
      description:
        "Início da descida. Você não volta como subiu: o envio para aplicar a transformação no seu lar, igreja e sociedade.",
      icon: <Send className="w-5 h-5 md:w-6 md:h-6" />,
    },
  ];

  return (
    <section id="jornada" className="py-24 bg-[#08110B] relative">
      <Container>
        <div className="max-w-3xl mx-auto text-center mb-16">
          <span className="text-[#F38B17] text-xs font-bold uppercase tracking-widest block mb-3">
            O Percurso
          </span>
          <h2 className="text-4xl sm:text-5xl md:text-6xl font-extrabold text-[#F5F5F5] tracking-tight uppercase mb-6">
            A <span className="text-[#3F7D45]">Jornada</span>
          </h2>
          <p className="text-lg text-[#A5A5A5]">
            Cada etapa da subida representa um marco de transformação na vida do homem.
          </p>
        </div>

        {/* Lista da Timeline */}
        <div className="max-w-3xl mx-auto">
          {steps.map((item, index) => (
            <TimelineItem
              key={item.step}
              step={item.step}
              title={item.title}
              description={item.description}
              icon={item.icon}
              isLast={index === steps.length - 1}
            />
          ))}
        </div>
      </Container>
    </section>
  );
};