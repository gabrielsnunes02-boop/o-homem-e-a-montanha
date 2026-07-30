"use client";

import React, { useState } from "react";
import { Input } from "@/components/ui/Input";
import { Button } from "@/components/ui/Button";
import { PixPaymentModal } from "./PixPaymentModal";
import { createRegistration } from "@/actions/register";
import { ShieldCheck, Loader2, AlertCircle } from "lucide-react";

export const RegistrationForm = () => {
  const [loading, setLoading] = useState(false);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [modalOpen, setModalOpen] = useState(false);
  const [pixResult, setPixResult] = useState<{
    registrationId: string;
    qrCode: string;
    qrCodeCopyPaste: string;
    amount: number;
  } | null>(null);

  const [formData, setFormData] = useState({
    fullName: "",
    phone: "",
    email: "",
    birthDate: "",
    church: "",
    city: "",
    emergencyName: "",
    emergencyPhone: "",
    medicalRestrictions: "",
    acceptedTerms: false,
  });

  const handleChange = (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    const { name, value, type } = e.target;
    const val = type === "checkbox" ? (e.target as HTMLInputElement).checked : value;
    setFormData((prev) => ({ ...prev, [name]: val }));
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setErrorMessage(null);

    if (!formData.acceptedTerms) {
      setErrorMessage("Você precisa aceitar os termos da expedição.");
      return;
    }

    setLoading(true);

    try {
      const res = await createRegistration(formData);

      if (res.success && res.data) {
        setPixResult(res.data);
        setModalOpen(true);
      } else {
        setErrorMessage(res.error || "Ocorreu um erro ao processar. Tente novamente.");
      }
    } catch {
      setErrorMessage("Erro imprevisto ao se conectar com o servidor.");
    } finally {
      setLoading(false);
    }
  };

  return (
    <>
      <form onSubmit={handleSubmit} className="flex flex-col gap-5">
        {errorMessage && (
          <div className="bg-red-950/60 border border-red-500/50 p-4 rounded-2xl flex items-center gap-3 text-red-200 text-sm">
            <AlertCircle className="w-5 h-5 shrink-0 text-red-400" />
            <span>{errorMessage}</span>
          </div>
        )}

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <Input
            label="Nome Completo *"
            name="fullName"
            placeholder="Seu nome completo"
            required
            value={formData.fullName}
            onChange={handleChange}
          />
          <Input
            label="WhatsApp / Telefone *"
            name="phone"
            placeholder="(85) 99999-9999"
            required
            value={formData.phone}
            onChange={handleChange}
          />
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <Input
            label="E-mail"
            name="email"
            type="email"
            placeholder="seu.email@exemplo.com"
            value={formData.email}
            onChange={handleChange}
          />
          <Input
            label="Data de Nascimento *"
            name="birthDate"
            type="date"
            required
            value={formData.birthDate}
            onChange={handleChange}
          />
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <Input
            label="Sua Igreja"
            name="church"
            placeholder="Nome da igreja que frequenta"
            value={formData.church}
            onChange={handleChange}
          />
          <Input
            label="Sua Cidade *"
            name="city"
            placeholder="Ex: Pacajús - CE"
            required
            value={formData.city}
            onChange={handleChange}
          />
        </div>

        <hr className="border-[#1E4D2B]/40 my-2" />

        <span className="text-xs font-bold text-[#F38B17] uppercase tracking-wider">
          Contato de Emergência
        </span>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <Input
            label="Nome do Contato *"
            name="emergencyName"
            placeholder="Ex: Esposa, Pai, Irmão"
            required
            value={formData.emergencyName}
            onChange={handleChange}
          />
          <Input
            label="Telefone de Emergência *"
            name="emergencyPhone"
            placeholder="(85) 99999-9999"
            required
            value={formData.emergencyPhone}
            onChange={handleChange}
          />
        </div>

        <div className="flex flex-col gap-2">
          <label className="text-sm font-medium text-[#A5A5A5] tracking-wide">
            Restrições Médicas ou Alergias (Opcional)
          </label>
          <textarea
            name="medicalRestrictions"
            rows={3}
            placeholder="Condição de saúde, alergia a medicamentos, etc."
            className="w-full bg-[#132416] border border-[#1E4D2B]/60 rounded-[16px] p-4 text-[#F5F5F5] placeholder-[#A5A5A5]/60 focus:outline-none focus:border-[#F38B17] transition-all duration-200 text-sm"
            value={formData.medicalRestrictions}
            onChange={handleChange}
          />
        </div>

        {/* Termos de Aceite */}
        <div className="flex items-start gap-3 mt-2">
          <input
            type="checkbox"
            id="acceptedTerms"
            name="acceptedTerms"
            required
            checked={formData.acceptedTerms}
            onChange={handleChange}
            className="mt-1 accent-[#F38B17] w-4 h-4 cursor-pointer"
          />
          <label htmlFor="acceptedTerms" className="text-xs text-[#A5A5A5] leading-relaxed cursor-pointer">
            Declaro estar ciente de que a expedição exige esforço físico e comprometo-me a seguir todas as orientações da equipe de liderança durante a subida.
          </label>
        </div>

        <Button
          type="submit"
          size="lg"
          disabled={loading}
          className="w-full mt-4 font-bold tracking-widest text-lg py-4 flex items-center justify-center gap-2 cursor-pointer"
        >
          {loading ? (
            <>
              <Loader2 className="w-5 h-5 animate-spin" /> GERANDO INSCRIÇÃO...
            </>
          ) : (
            <>
              <ShieldCheck className="w-5 h-5" /> CONFIRMAR E GERAR PIX
            </>
          )}
        </Button>
      </form>

      {/* Modal do QR Code PIX */}
      <PixPaymentModal
        isOpen={modalOpen}
        onClose={() => setModalOpen(false)}
        pixData={pixResult}
      />
    </>
  );
};