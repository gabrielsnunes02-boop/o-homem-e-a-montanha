"use client";

import React from "react";
import { Container } from "@/components/layout/Container";
import { Card } from "@/components/ui/Card";
import { Progress } from "@/components/ui/Progress";
import { Badge } from "@/components/ui/Badge";
import { RegistrationForm } from "@/components/registration/RegistrationForm";
import { Calendar, MapPin, DollarSign, Users } from "lucide-react";

export const RegistrationSection = () => {
  // Valores estáticos de demonstração até ligarmos ao banco
  const maxParticipants = 50;
  const currentRegistrations = 32;
  const percentage = Math.round((currentRegistrations / maxParticipants) * 100);

  return (
    <section id="inscricao" className="py-24 bg-[#08110B] relative">
      <Container>
        <div className="max-w-3xl mx-auto text-center mb-12">
          <span className="text-[#F38B17] text-xs font-bold uppercase tracking-widest block mb-3">
            Inscrições Abertas
          </span>
          <h2 className="text-4xl sm:text-5xl font-extrabold text-[#F5F5F5] tracking-tight uppercase mb-4">
            Garanta seu lugar na <span className="text-[#F38B17]">Montanha</span>
          </h2>
          <p className="text-[#A5A5A5]">
            Preencha seus dados para assegurar sua vaga nesta expedição.
          </p>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-start">
          {/* Card de Detalhes da Vaga e Evento */}
          <div className="lg:col-span-5 flex flex-col gap-6">
            <Card className="bg-[#132416] border-[#1E4D2B] p-6">
              <div className="flex items-center justify-between mb-4">
                <span className="text-sm font-semibold text-[#F5F5F5] uppercase">
                  Vagas Preenchidas
                </span>
                <Badge variant="orange">{percentage}% VAGAS</Badge>
              </div>

              <Progress value={percentage} className="mb-4" />

              <div className="flex justify-between text-xs text-[#A5A5A5] font-medium">
                <span>{currentRegistrations} Confirmados</span>
                <span>{maxParticipants - currentRegistrations} Vagas Restantes</span>
              </div>
            </Card>

            <Card className="bg-[#132416] border-[#1E4D2B]/50 p-6 flex flex-col gap-5">
              <h3 className="text-lg font-bold text-[#F5F5F5] uppercase border-b border-[#1E4D2B]/40 pb-3">
                Informações do Evento
              </h3>

              <div className="flex items-start gap-4">
                <div className="p-2.5 bg-[#1E4D2B]/30 rounded-xl text-[#F38B17] shrink-0">
                  <Calendar className="w-5 h-5" />
                </div>
                <div>
                  <span className="block text-xs text-[#A5A5A5] uppercase font-medium">Data & Horário</span>
                  <span className="text-sm font-semibold text-[#F5F5F5]">Em Breve (Expedição Noturna)</span>
                </div>
              </div>

              <div className="flex items-start gap-4">
                <div className="p-2.5 bg-[#1E4D2B]/30 rounded-xl text-[#3F7D45] shrink-0">
                  <MapPin className="w-5 h-5" />
                </div>
                <div>
                  <span className="block text-xs text-[#A5A5A5] uppercase font-medium">Concentração</span>
                  <span className="text-sm font-semibold text-[#F5F5F5]">Igreja Viva em Missões</span>
                </div>
              </div>

              <div className="flex items-start gap-4">
                <div className="p-2.5 bg-[#1E4D2B]/30 rounded-xl text-[#F38B17] shrink-0">
                  <DollarSign className="w-5 h-5" />
                </div>
                <div>
                  <span className="block text-xs text-[#A5A5A5] uppercase font-medium">Investimento</span>
                  <span className="text-lg font-extrabold text-[#F38B17]">R$ 50,00</span>
                </div>
              </div>

              <div className="flex items-start gap-4">
                <div className="p-2.5 bg-[#1E4D2B]/30 rounded-xl text-[#3F7D45] shrink-0">
                  <Users className="w-5 h-5" />
                </div>
                <div>
                  <span className="block text-xs text-[#A5A5A5] uppercase font-medium">Público</span>
                  <span className="text-sm font-semibold text-[#F5F5F5]">Homens (A partir de 18 anos)</span>
                </div>
              </div>
            </Card>
          </div>

          {/* Card do Formulário */}
          <div className="lg:col-span-7">
            <Card className="bg-[#132416] border-[#1E4D2B] p-6 sm:p-8">
              <RegistrationForm />
            </Card>
          </div>
        </div>
      </Container>
    </section>
  );
};