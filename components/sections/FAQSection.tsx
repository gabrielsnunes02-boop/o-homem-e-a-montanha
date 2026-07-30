"use client";

import React, { useState } from "react";
import { Container } from "@/components/layout/Container";
import { Card } from "@/components/ui/Card";
import { ChevronDown, HelpCircle } from "lucide-react";
import { cn } from "@/lib/utils/cn";

interface FAQItem {
  question: string;
  answer: string;
}

const faqs: FAQItem[] = [
  {
    question: "Qual é o nível de dificuldade física da caminhada?",
    answer:
      "A subida exige um nível de esforço moderado a intenso. Caminharemos em ritmo contínuo com paradas estratégicas. É importante estar preparado para uma experiência física e espiritual desafiadora.",
  },
  {
    question: "Existe idade mínima para participar?",
    answer:
      "Sim. A participação é recomendada para homens a partir de 18 anos. Menores a partir de 16 anos só poderão participar acompanhados pelo pai ou responsável legal mediante autorização por escrito.",
  },
  {
    question: "O que está incluso na taxa de inscrição?",
    answer:
      "A taxa cobre a infraestrutura de apoio, equipe de segurança/primeiros socorros, materiais de apoio da expedição e a estrutura de recepção ao final.",
  },
  {
    question: "Como funciona a alimentação e hidratação?",
    answer:
      "Cada participante deve levar sua própria garrafa de água para a subida e lanches práticos de rápida absorção (barras de cereal, castanhas, frutas secas). Teremos pontos de apoio de água ao longo do percurso.",
  },
  {
    question: "E se chover no dia do evento?",
    answer:
      "O evento é mantido sob condições de chuva leve. Caso haja condições meteorológicas extremas ou risco à segurança dos participantes, a organização informará o reagendamento nos canais oficiais.",
  },
  {
    question: "Quais são as formas de pagamento disponíveis?",
    answer:
      "Aceitamos pagamento via PIX (com aprovação imediata para garantia da vaga) e Cartão de Crédito.",
  },
];

export const FAQSection = () => {
  const [openIndex, setOpenIndex] = useState<number | null>(null);

  const toggleFAQ = (index: number) => {
    setOpenIndex(openIndex === index ? null : index);
  };

  return (
    <section id="faq" className="py-24 bg-[#0D1B11] border-t border-[#1E4D2B]/30 relative">
      <Container className="max-w-4xl">
        <div className="text-center mb-16">
          <span className="text-[#F38B17] text-xs font-bold uppercase tracking-widest block mb-3">
            Tire Suas Dúvidas
          </span>
          <h2 className="text-4xl sm:text-5xl font-extrabold text-[#F5F5F5] tracking-tight uppercase mb-4">
            Perguntas <span className="text-[#3F7D45]">Frequentes</span>
          </h2>
          <p className="text-[#A5A5A5]">
            Tudo o que você precisa saber antes de iniciar a subida.
          </p>
        </div>

        <div className="flex flex-col gap-4">
          {faqs.map((faq, index) => {
            const isOpen = openIndex === index;
            return (
              <Card
                key={index}
                className="bg-[#132416]/80 border-[#1E4D2B]/50 p-0 overflow-hidden cursor-pointer transition-all duration-300"
                onClick={() => toggleFAQ(index)}
              >
                <div className="p-6 flex items-center justify-between gap-4 select-none">
                  <div className="flex items-center gap-3">
                    <HelpCircle className="w-5 h-5 text-[#F38B17] shrink-0" />
                    <h3 className="text-base sm:text-lg font-bold text-[#F5F5F5]">
                      {faq.question}
                    </h3>
                  </div>
                  <ChevronDown
                    className={cn(
                      "w-5 h-5 text-[#3F7D45] transition-transform duration-300 shrink-0",
                      isOpen && "rotate-180 text-[#F38B17]"
                    )}
                  />
                </div>

                {isOpen && (
                  <div className="px-6 pb-6 pt-0 text-sm sm:text-base text-[#A5A5A5] leading-relaxed border-t border-[#1E4D2B]/30 pt-4">
                    {faq.answer}
                  </div>
                )}
              </Card>
            );
          })}
        </div>
      </Container>
    </section>
  );
};