"use client";

import React, { useState, useEffect } from "react";
import { Check, Copy, MessageCircle, X, Loader2 } from "lucide-react";
import { Button } from "@/components/ui/Button";
import { checkPaymentStatus } from "@/actions/register";

interface PixModalProps {
  isOpen: boolean;
  onClose: () => void;
  pixData: {
    registrationId: string;
    qrCodeBase64?: string;
    qrCodeCopyPaste: string;
    amount: number;
  } | null;
}

export function PixPaymentModal({ isOpen, onClose, pixData }: PixModalProps) {
  const [copied, setCopied] = useState(false);
  const [isPaid, setIsPaid] = useState(false);

  // Polling: Consulta o Supabase a cada 4 segundos para saber se mudou para APPROVED
  useEffect(() => {
    if (!isOpen || !pixData?.registrationId || isPaid) return;

    const interval = setInterval(async () => {
      const res = await checkPaymentStatus(pixData.registrationId);
      if (res.isApproved) {
        setIsPaid(true);
        clearInterval(interval);
      }
    }, 4000);

    return () => clearInterval(interval);
  }, [isOpen, pixData?.registrationId, isPaid]);

  if (!isOpen || !pixData) return null;

  const handleCopy = () => {
    navigator.clipboard.writeText(pixData.qrCodeCopyPaste);
    setCopied(true);
    setTimeout(() => setCopied(false), 3000);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/80 backdrop-blur-sm p-4">
      <div className="bg-[#132416] border border-[#1E4D2B] rounded-2xl w-full max-w-md p-6 text-center relative shadow-2xl">
        <button
          onClick={onClose}
          className="absolute top-4 right-4 text-[#A5A5A5] hover:text-white transition-colors"
        >
          <X className="w-5 h-5" />
        </button>

        {!isPaid ? (
          /* --- ETAPA 1: PENDENTE DE PAGAMENTO --- */
          <>
            <h3 className="text-xl font-extrabold uppercase text-[#F5F5F5] mb-1">
              Pagamento via PIX
            </h3>
            <p className="text-xs text-[#A5A5A5] mb-4">
              Escaneie o QR Code abaixo ou copie o código PIX para realizar o pagamento.
            </p>

            {/* QR Code dinâmico do Mercado Pago */}
            {pixData.qrCodeBase64 && (
              <div className="bg-white p-3 rounded-xl inline-block mb-4 shadow-inner">
                <img
                  src={`data:image/png;base64,${pixData.qrCodeBase64}`}
                  alt="QR Code PIX Mercado Pago"
                  className="w-48 h-48 mx-auto object-contain"
                />
              </div>
            )}

            <div className="bg-[#08110B] border border-[#1E4D2B]/60 rounded-xl p-3 mb-4">
              <span className="text-[10px] text-[#A5A5A5] uppercase font-bold block mb-1">
                Valor Total
              </span>
              <span className="text-2xl font-extrabold text-[#F38B17]">
                R$ {pixData.amount.toFixed(2).replace(".", ",")}
              </span>
            </div>

            {/* Botão Copia e Cola */}
            <Button
              onClick={handleCopy}
              className="w-full bg-[#F38B17] hover:bg-[#d9780e] text-black font-extrabold py-3 rounded-xl flex items-center justify-center gap-2 mb-4 cursor-pointer"
            >
              {copied ? (
                <>
                  <Check className="w-4 h-4" /> CÓDIGO COPIADO!
                </>
              ) : (
                <>
                  <Copy className="w-4 h-4" /> COPIAR CÓDIGO PIX
                </>
              )}
            </Button>

            {/* AVISO DE AGUARDANDO CONFIRMAÇÃO DO BANCO */}
            <div className="bg-[#08110B] border border-[#1E4D2B]/80 rounded-xl p-3 flex items-center gap-3 text-left">
              <Loader2 className="w-5 h-5 text-[#F38B17] animate-spin shrink-0" />
              <p className="text-[11px] text-[#A5A5A5] leading-tight">
                Aguardando confirmação do PIX... O botão para o grupo oficial do WhatsApp será liberado aqui automaticamente após o pagamento.
              </p>
            </div>
          </>
        ) : (
          /* --- ETAPA 2: PAGAMENTO APROVADO (WHATSAPP LIBERADO) --- */
          <div className="py-4 animate-fade-in">
            <div className="w-16 h-16 bg-[#1E4D2B] text-[#25D366] border border-[#25D366]/40 rounded-full flex items-center justify-center mx-auto mb-4 text-3xl font-extrabold shadow-lg">
              ✓
            </div>
            <h3 className="text-2xl font-extrabold uppercase text-[#F5F5F5] mb-2">
              Inscrição Confirmada!
            </h3>
            <p className="text-xs text-[#A5A5A5] mb-6 max-w-xs mx-auto leading-relaxed">
              Seu pagamento foi identificado com sucesso! Clique abaixo para garantir sua entrada no grupo oficial dos participantes:
            </p>

            {/* BOTÃO DO WHATSAPP (SÓ EXISTE SE ISPAID FOR TRUE) */}
            <a
              href="https://chat.whatsapp.com/IgZIOybgkgB7UMrq7WUBFM?s=cl&p=a&mlu=0"
              target="_blank"
              rel="noopener noreferrer"
              className="w-full bg-[#25D366] hover:bg-[#20ba5a] text-black font-extrabold py-3.5 rounded-xl flex items-center justify-center gap-2 text-sm transition-transform active:scale-95 shadow-xl"
            >
              <MessageCircle className="w-5 h-5" /> ENTRAR NO GRUPO DO WHATSAPP
            </a>
          </div>
        )}
      </div>
    </div>
  );
}