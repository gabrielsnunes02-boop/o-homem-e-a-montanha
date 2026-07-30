"use client";

import React, { useEffect, useState, useCallback } from "react";
import { Container } from "@/components/layout/Container";
import { Card } from "@/components/ui/Card";
import { Badge } from "@/components/ui/Badge";
import { Button } from "@/components/ui/Button";
import { getRegistrations, updateRegistrationStatus, RegistrationStatus } from "@/actions/admin";
import {
  Users,
  CheckCircle2,
  Clock,
  DollarSign,
  Mountain,
  RefreshCw,
  Search,
} from "lucide-react";

interface Registration {
  id: string;
  fullName: string;
  phone: string;
  email: string | null;
  city: string;
  church: string | null;
  status: RegistrationStatus;
  createdAt: Date | string;
}

export default function AdminDashboard() {
  const [data, setData] = useState<Registration[]>([]);
  const [loading, setLoading] = useState(true);
  const [search, setSearch] = useState("");
  const [filterStatus, setFilterStatus] = useState<string>("ALL");

  const loadData = useCallback(async () => {
    setLoading(true);
    const res = await getRegistrations();
    if (res.success && res.data) {
      setData(res.data as Registration[]);
    }
    setLoading(false);
  }, []);

  useEffect(() => {
    let isMounted = true;
    
    getRegistrations().then((res) => {
      if (isMounted) {
        if (res.success && res.data) {
          setData(res.data as Registration[]);
        }
        setLoading(false);
      }
    });

    return () => {
      isMounted = false;
    };
  }, []);

  const handleStatusChange = async (id: string, newStatus: RegistrationStatus) => {
    const res = await updateRegistrationStatus(id, newStatus);
    if (res.success) {
      setData((prev) =>
        prev.map((item) => (item.id === id ? { ...item, status: newStatus } : item))
      );
    }
  };

  // Métricas
  const total = data.length;
  const confirmed = data.filter((i) => i.status === "CONFIRMED").length;
  const pending = data.filter((i) => i.status === "PENDING").length;
  const revenue = confirmed * 50;

  // Filtros
  const filteredData = data.filter((item) => {
    const matchesSearch =
      item.fullName.toLowerCase().includes(search.toLowerCase()) ||
      item.phone.includes(search) ||
      item.city.toLowerCase().includes(search.toLowerCase());

    const matchesStatus =
      filterStatus === "ALL" ? true : item.status === filterStatus;

    return matchesSearch && matchesStatus;
  });

  return (
    <div className="min-h-screen bg-[#08110B] text-[#F5F5F5] py-10">
      <Container>
        {/* Header Admin */}
        <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4 mb-8 border-b border-[#1E4D2B]/40 pb-6">
          <div className="flex items-center gap-3">
            <div className="p-3 bg-[#132416] border border-[#1E4D2B] rounded-2xl text-[#F38B17]">
              <Mountain className="w-7 h-7" />
            </div>
            <div>
              <h1 className="text-2xl font-extrabold uppercase tracking-tight">
                Painel do Organizador
              </h1>
              <p className="text-xs text-[#A5A5A5]">
                O Homem e a Montanha &bull; Gestão de Inscrições
              </p>
            </div>
          </div>

          <Button
            onClick={loadData}
            variant="outline"
            className="flex items-center gap-2 text-xs py-2 px-4 cursor-pointer"
          >
            <RefreshCw className={`w-4 h-4 ${loading ? "animate-spin" : ""}`} />
            Atualizar Dados
          </Button>
        </div>

        {/* Métricas / Cards */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-8">
          <Card className="bg-[#132416] border-[#1E4D2B] p-5">
            <div className="flex items-center justify-between text-[#A5A5A5] mb-2">
              <span className="text-xs font-bold uppercase">Total Inscritos</span>
              <Users className="w-5 h-5 text-[#F38B17]" />
            </div>
            <span className="text-3xl font-extrabold text-[#F5F5F5]">{total}</span>
          </Card>

          <Card className="bg-[#132416] border-[#1E4D2B] p-5">
            <div className="flex items-center justify-between text-[#A5A5A5] mb-2">
              <span className="text-xs font-bold uppercase">Confirmados</span>
              <CheckCircle2 className="w-5 h-5 text-green-400" />
            </div>
            <span className="text-3xl font-extrabold text-green-400">{confirmed}</span>
          </Card>

          <Card className="bg-[#132416] border-[#1E4D2B] p-5">
            <div className="flex items-center justify-between text-[#A5A5A5] mb-2">
              <span className="text-xs font-bold uppercase">Aguardando PIX</span>
              <Clock className="w-5 h-5 text-yellow-400" />
            </div>
            <span className="text-3xl font-extrabold text-yellow-400">{pending}</span>
          </Card>

          <Card className="bg-[#132416] border-[#1E4D2B] p-5">
            <div className="flex items-center justify-between text-[#A5A5A5] mb-2">
              <span className="text-xs font-bold uppercase">Arrecadado</span>
              <DollarSign className="w-5 h-5 text-[#F38B17]" />
            </div>
            <span className="text-3xl font-extrabold text-[#F38B17]">
              R$ {revenue.toFixed(2).replace(".", ",")}
            </span>
          </Card>
        </div>

        {/* Filtros e Busca */}
        <Card className="bg-[#132416] border-[#1E4D2B] p-4 mb-6 flex flex-col sm:flex-row gap-4 justify-between items-center">
          <div className="relative w-full sm:w-80">
            <Search className="w-4 h-4 absolute left-3 top-1/2 -translate-y-1/2 text-[#A5A5A5]" />
            <input
              type="text"
              placeholder="Buscar por nome, fone ou cidade..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              className="w-full pl-9 pr-4 py-2 bg-[#08110B] border border-[#1E4D2B] rounded-xl text-xs text-[#F5F5F5] placeholder-[#A5A5A5]/60 focus:outline-none focus:border-[#F38B17]"
            />
          </div>

          <div className="flex items-center gap-2 w-full sm:w-auto">
            <button
              onClick={() => setFilterStatus("ALL")}
              className={`text-xs px-3 py-2 rounded-xl font-bold uppercase transition-all ${
                filterStatus === "ALL"
                  ? "bg-[#1E4D2B] text-white"
                  : "bg-[#08110B] text-[#A5A5A5] border border-[#1E4D2B]/40"
              }`}
            >
              Todos
            </button>
            <button
              onClick={() => setFilterStatus("CONFIRMED")}
              className={`text-xs px-3 py-2 rounded-xl font-bold uppercase transition-all ${
                filterStatus === "CONFIRMED"
                  ? "bg-green-700 text-white"
                  : "bg-[#08110B] text-[#A5A5A5] border border-[#1E4D2B]/40"
              }`}
            >
              Confirmados
            </button>
            <button
              onClick={() => setFilterStatus("PENDING")}
              className={`text-xs px-3 py-2 rounded-xl font-bold uppercase transition-all ${
                filterStatus === "PENDING"
                  ? "bg-yellow-700 text-white"
                  : "bg-[#08110B] text-[#A5A5A5] border border-[#1E4D2B]/40"
              }`}
            >
              Pendentes
            </button>
          </div>
        </Card>

        {/* Tabela de Inscrições */}
        <Card className="bg-[#132416] border-[#1E4D2B] p-0 overflow-x-auto">
          <table className="w-full text-left text-xs text-[#A5A5A5]">
            <thead className="bg-[#08110B] uppercase font-bold text-[#F5F5F5] border-b border-[#1E4D2B]/60">
              <tr>
                <th className="p-4">Nome</th>
                <th className="p-4">Contato</th>
                <th className="p-4">Cidade / Igreja</th>
                <th className="p-4">Status</th>
                <th className="p-4 text-right">Ação</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-[#1E4D2B]/30">
              {filteredData.length === 0 ? (
                <tr>
                  <td colSpan={5} className="p-8 text-center text-[#A5A5A5]">
                    Nenhuma inscrição encontrada.
                  </td>
                </tr>
              ) : (
                filteredData.map((item) => (
                  <tr key={item.id} className="hover:bg-[#1E4D2B]/20 transition-colors">
                    <td className="p-4 font-bold text-[#F5F5F5]">{item.fullName}</td>
                    <td className="p-4">
                      <span className="block text-[#F5F5F5]">{item.phone}</span>
                      <span className="text-[10px] text-[#A5A5A5]">{item.email || "-"}</span>
                    </td>
                    <td className="p-4">
                      <span className="block text-[#F5F5F5]">{item.city}</span>
                      <span className="text-[10px] text-[#3F7D45]">{item.church || "Não informada"}</span>
                    </td>
                    <td className="p-4">
                      {item.status === "CONFIRMED" && (
                        <Badge variant="green">CONFIRMADO</Badge>
                      )}
                      {item.status === "PENDING" && (
                        <Badge variant="orange">PENDENTE</Badge>
                      )}
                      {item.status === "CANCELLED" && (
                        <Badge variant="gray">CANCELADO</Badge>
                      )}
                    </td>
                    <td className="p-4 text-right">
                      {item.status === "PENDING" && (
                        <Button
                          size="sm"
                          onClick={() => handleStatusChange(item.id, "CONFIRMED")}
                          className="text-[10px] py-1 px-3 bg-green-600 hover:bg-green-700 font-bold"
                        >
                          Confirmar PIX
                        </Button>
                      )}
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </Card>
      </Container>
    </div>
  );
}