"use server";

import { db } from "@/lib/db";
import { registrations } from "@/lib/db/schema";

export interface RegistrationInput {
  fullName: string;
  phone: string;
  email?: string;
  birthDate: string;
  church?: string;
  city: string;
  emergencyName: string;
  emergencyPhone: string;
  medicalRestrictions?: string;
}

export async function createRegistration(data: RegistrationInput) {
  try {
    // Gerar código único para a inscrição (ex: HM-8X92)
    const registrationCode = `HM-${Math.random().toString(36).substring(2, 7).toUpperCase()}`;

    // 1. Inserir no Banco via Drizzle ORM
    const [newRegistration] = await db
      .insert(registrations)
      .values({
        registrationCode,
        eventId: "event-01", // ID padrão para o evento atual
        fullName: data.fullName,
        phone: data.phone,
        email: data.email || null,
        birthDate: data.birthDate ? new Date(data.birthDate) : new Date(),
        church: data.church || null,
        city: data.city,
        emergencyName: data.emergencyName,
        emergencyPhone: data.emergencyPhone,
        medicalRestrictions: data.medicalRestrictions || null,
        status: "PENDING",
      })
      .returning();

    // 2. Payload da cobrança PIX
    const pixData = {
      registrationId: newRegistration.id,
      qrCode: "https://api.qrserver.com/v1/create-qr-code/?size=250x250&data=HOMEM-MONTANHA-PIX-MOCK-PAYMENT",
      qrCodeCopyPaste: "00020126580014br.gov.bcb.pix0136123e4567-e89b-12d3-a456-426614174000520400005303986540550.005802BR5922IGREJA VIVA EM MISSOES6007PACAJUS62070503***6304E2CA",
      amount: 50.00,
    };

    return {
      success: true,
      data: pixData,
    };
  } catch (error) {
    console.error("Erro ao processar inscrição:", error);
    return {
      success: false,
      error: "Ocorreu um erro ao salvar sua inscrição. Tente novamente.",
    };
  }
}