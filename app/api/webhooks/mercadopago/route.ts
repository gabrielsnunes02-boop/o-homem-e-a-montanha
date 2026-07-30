import { NextResponse } from "next/server";
import { db } from "@/lib/db";
import { registrations } from "@/lib/db/schema";
import { eq } from "drizzle-orm";
import { MercadoPagoConfig, Payment } from "mercadopago";

// Inicializa o cliente com o mesmo token do .env.local
const client = new MercadoPagoConfig({
  accessToken: process.env.MERCADOPAGO_ACCESS_TOKEN || "",
});

export async function POST(request: Request) {
  try {
    const url = new URL(request.url);
    
    // O Mercado Pago envia o ID do pagamento via Query Params ou Body
    const dataId = url.searchParams.get("data.id") || url.searchParams.get("id");
    const type = url.searchParams.get("type");

    // Processa apenas se for uma notificação de pagamento e possuir um ID
    if ((type === "payment" || !type) && dataId) {
      const payment = new Payment(client);
      const paymentData = await payment.get({ id: dataId });

      // Se o pagamento no Mercado Pago foi APROVADO:
      if (paymentData.status === "approved") {
        const description = paymentData.description || "";
        
        // Extrai o código de inscrição no formato HM-XXXXX da descrição
        const codeMatch = description.match(/HM-[A-Z0-9]+/);

        if (codeMatch) {
          const registrationCode = codeMatch[0];

          // Atualiza o status no Supabase de PENDING para APPROVED
          await db
            .update(registrations)
            .set({
              status: "APPROVED",
            })
            .where(eq(registrations.registrationCode, registrationCode));

          console.log(`✅ Inscrição ${registrationCode} aprovada com sucesso!`);
        }
      }
    }

    // O Mercado Pago exige resposta HTTP 200/201 para saber que a notificação foi recebida
    return NextResponse.json({ received: true }, { status: 200 });
  } catch (error) {
    console.error("Erro ao processar Webhook do Mercado Pago:", error);
    // Retornamos 200 mesmo em caso de erro interno para evitar re-tentativas em loop do MP
    return NextResponse.json({ received: true, error: "Internal processing error" }, { status: 200 });
  }
}