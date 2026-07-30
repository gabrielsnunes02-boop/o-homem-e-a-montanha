"use server";

import { db } from "@/lib/db";
import { registrations } from "@/lib/db/schema";
import { MercadoPagoConfig, Payment } from "mercadopago";
import { randomUUID } from "crypto";
import { eq } from "drizzle-orm"; // <-- 1. IMPORTANTE: Import do 'eq' adicionado

// Inicializa o cliente do Mercado Pago com a chave do .env.local
const client = new MercadoPagoConfig({
  accessToken: process.env.MERCADOPAGO_ACCESS_TOKEN || "",
});

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
    const registrationCode = `HM-${Math.random().toString(36).substring(2, 7).toUpperCase()}`;

    // Trata a data com segurança para não quebrar a inserção
    let parsedBirthDate = new Date();
    if (data.birthDate) {
      const dateObj = new Date(data.birthDate);
      if (!isNaN(dateObj.getTime())) {
        parsedBirthDate = dateObj;
      }
    }

    // 1. Inserir a inscrição no Supabase (Status PENDING)
    const [newRegistration] = await db
      .insert(registrations)
      .values({
        registrationCode,
        eventId: "event-01",
        fullName: data.fullName,
        phone: data.phone,
        email: data.email || null,
        birthDate: parsedBirthDate,
        church: data.church || null,
        city: data.city,
        emergencyName: data.emergencyName,
        emergencyPhone: data.emergencyPhone,
        medicalRestrictions: data.medicalRestrictions || null,
        acceptedTerms: true,
        status: "PENDING",
      })
      .returning();

    // 2. Tratar o nome para o Mercado Pago
    const nameParts = data.fullName.trim().split(" ");
    const firstName = nameParts[0];
    const lastName = nameParts.length > 1 ? nameParts.slice(1).join(" ") : "Inscrito";

    // 3. Gerar a cobrança PIX no Mercado Pago com Idempotency Key
    const payment = new Payment(client);

    // Identifica se estamos rodando com chave de teste ou produção
    const isTestToken = process.env.MERCADOPAGO_ACCESS_TOKEN?.startsWith("TEST-");

    // Em ambiente TEST, usamos o email padrão de sandbox exigido pela API
    const payerEmail = isTestToken
      ? "test_user_1234567@testuser.com"
      : (data.email || `${registrationCode.toLowerCase()}@homem-montanha.com`);

    const paymentResponse = await payment.create({
      body: {
        transaction_amount: 1.0,
        description: `Inscrição Expedição O Homem e a Montanha - ${registrationCode}`,
        payment_method_id: "pix",
        payer: {
          email: payerEmail,
          first_name: firstName,
          last_name: lastName,
        },
      },
      requestOptions: {
        idempotencyKey: randomUUID(),
      },
    });

    const pointOfInteraction = paymentResponse.point_of_interaction?.transaction_data;

    if (pointOfInteraction) {
      return {
        success: true,
        data: {
          registrationId: newRegistration.id,
          qrCodeBase64: pointOfInteraction.qr_code_base64,
          qrCodeCopyPaste: pointOfInteraction.qr_code,
          amount: 1.0,
        },
      };
    }

    throw new Error("Não foi possível gerar os dados do QR Code no Mercado Pago.");
  } catch (error: any) {
    console.log("----------------------------------------");
    console.log("ERRO DETALHADO:");
    console.log("Message:", error?.message);
    console.log("Cause:", JSON.stringify(error?.cause || error, null, 2));
    console.log("----------------------------------------");

    return {
      success: false,
      error: error?.message || "Erro ao processar a inscrição.",
    };
  }
}

// 2. ADICIONADO: Função para consultar o status de pagamento pelo ID do registro
export async function checkPaymentStatus(registrationId: string) {
  try {
    const [registration] = await db
      .select({ status: registrations.status })
      .from(registrations)
      .where(eq(registrations.id, registrationId));

    return {
      status: registration?.status || "PENDING",
      isApproved: registration?.status === "CONFIRMED",
    };
  } catch (error) {
    return { status: "PENDING", isApproved: false };
  }
}