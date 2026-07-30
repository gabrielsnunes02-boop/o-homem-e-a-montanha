"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { Card } from "@/components/ui/Card";
import { Flashlight, Shield, BookOpen, Droplets, Shirt, Backpack } from "lucide-react";

export const EquipmentSection = () => {
  const items = [
    {
      title: "Mochila Leve",
      description: "Apenas o essencial para manter as mãos livres durante a subida.",
      icon: <Backpack className="w-7 h-7 text-[#F38B17]" />,
    },
    {
      title: "Lanterna de Cabeça ou Mão",
      description: "Item obrigatório para a caminhada e segurança no trecho noturno.",
      icon: <Flashlight className="w-7 h-7 text-[#3F7D45]" />,
    },
    {
      title: "Garrafa de Água",
      description: "Hidratação constante durante toda a expedição.",
      icon: <Droplets className="w-7 h-7 text-[#F38B17]" />,
    },
    {
      title: "Bíblia e Bloco de Notas",
      description: "Sua palavra e papel para registrar as ministrações e direcionamentos.",
      icon: <BookOpen className="w-7 h-7 text-[#3F7D45]" />,
    },
    {
      title: "Roupas e Calçados Adequados",
      description: "Tênis/bota de trilha confortável e agasalho para o frio da montanha.",
      icon: <Shirt className="w-7 h-7 text-[#F38B17]" />,
    },
    {
      title: "Coração Preparado",
      description: "Abertura para ser confrontado, moldado e transformado por Deus.",
      icon: <Shield className="w-7 h-7 text-[#3F7D45]" />,
    },
  ];

  return (
    <section className="py-24 bg-[#08110B] relative">
      <Container>
        <div className="max-w-3xl mx-auto text-center mb-16">
          <span className="text-[#F38B17] text-xs font-bold uppercase tracking-widest block mb-3">
            Preparação
          </span>
          <h2 className="text-4xl sm:text-5xl md:text-6xl font-extrabold text-[#F5F5F5] tracking-tight uppercase mb-6">
            O Que <span className="text-[#3F7D45]">Levar</span>
          </h2>
          <p className="text-lg text-[#A5A5A5]">
            Para garantir uma experiência segura e proveitosa, esteja equipado adequadamente.
          </p>
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6">
          {items.map((item, index) => (
            <Card
              key={index}
              className="bg-[#132416]/70 border-[#1E4D2B]/40 hover:border-[#3F7D45] transition-all duration-300 p-6 flex flex-col gap-4"
            >
              <div className="p-3 bg-[#1E4D2B]/30 rounded-xl w-fit">
                {item.icon}
              </div>
              <h3 className="text-xl font-bold text-[#F5F5F5] uppercase">
                {item.title}
              </h3>
              <p className="text-[#A5A5A5] text-sm leading-relaxed">
                {item.description}
              </p>
            </Card>
          ))}
        </div>
      </Container>
    </section>
  );
};