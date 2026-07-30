"use client";

import React, { useState } from "react";
import { Card } from "@/components/ui/Card";
import { Button } from "@/components/ui/Button";
import { Check, Copy, QrCode, X } from "lucide-react";

interface PixPaymentModalProps {
  isOpen: boolean;
  onClose: () => void;
  pixData: {
    registrationId: string;
    qrCode: string;
    qrCodeCopyPaste: string;
    amount: number;
  } | null;
}

export const PixPaymentModal = ({ isOpen, onClose, pixData }: PixPaymentModalProps) => {
  const [copied, setCopied] = useState(false);

  if (!isOpen || !pixData) return null;

  const handleCopy = () => {
    navigator.clipboard.writeText(pixData.qrCodeCopyPaste);
    setCopied(true);
    setTimeout(() => setCopied(false), 3000);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm animate-in fade-in duration-200">
      <Card className="bg-[#132416] border-[#1E4D2B] p-6 sm:p-8 max-w-md w-full relative shadow-2xl">
        <button
          onClick={onClose}
          className="absolute top-4 right-4 text-[#A5A5A5] hover:text-white transition-colors cursor-pointer"
        >
          <X className="w-5 h-5" />
        </button>

        <div className="text-center mb-6">
          <div className="w-12 h-12 bg-[#1E4D2B]/30 text-[#F38B17] rounded-full flex items-center justify-center mx-auto mb-3">
            <QrCode className="w-6 h-6" />
          </div>
          <h3 className="text-2xl font-bold text-[#F5F5F5] uppercase">
            Inscrição Realizada!
          </h3>
          <p className="text-xs text-[#A5A5A5] mt-1">
            Escaneie o QR Code abaixo ou copie a chave PIX para concluir o pagamento.
          </p>
        </div>

        {/* Imagem do QR Code */}
        <div className="bg-white p-4 rounded-2xl w-fit mx-auto mb-6 border-2 border-[#1E4D2B]">
          {/* eslint-disable-next-line @next/next/no-img-element */}
          <img
            src={pixData.qrCode}
            alt="QR Code PIX"
            className="w-48 h-48 object-contain"
          />
        </div>

        {/* Valor */}
        <div className="text-center mb-6">
          <span className="text-xs text-[#A5A5A5] uppercase font-semibold block">Valor a Pagar</span>
          <span className="text-3xl font-extrabold text-[#F38B17]">
            R$ {pixData.amount.toFixed(2).replace(".", ",")}
          </span>
        </div>

        {/* PIX Copia e Cola */}
        <div className="flex flex-col gap-2">
          <label className="text-xs text-[#A5A5A5] uppercase font-bold">
            PIX Copia e Cola
          </label>
          <div className="flex items-center gap-2">
            <input
              type="text"
              readOnly
              value={pixData.qrCodeCopyPaste}
              className="bg-[#08110B] border border-[#1E4D2B] rounded-xl p-3 text-xs text-[#A5A5A5] flex-1 truncate focus:outline-none"
            />
            <Button
              onClick={handleCopy}
              className="px-4 py-3 text-xs font-bold flex items-center gap-1 shrink-0 cursor-pointer"
            >
              {copied ? (
                <>
                  <Check className="w-4 h-4 text-green-400" /> COPIADO
                </>
              ) : (
                <>
                  <Copy className="w-4 h-4" /> COPIAR
                </>
              )}
            </Button>
          </div>
        </div>

        <p className="text-[11px] text-[#A5A5A5]/70 text-center mt-6">
          Após a confirmação do pagamento, seu comprovante será processado e a vaga estará 100% garantida.
        </p>
      </Card>
    </div>
  );
};