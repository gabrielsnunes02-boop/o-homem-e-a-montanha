"use server";

import { db } from "@/lib/db";
import { registrations } from "@/lib/db/schema";
import { eq, desc } from "drizzle-orm";

export type RegistrationStatus = "PENDING" | "CONFIRMED" | "CANCELLED" | "WAITING_LIST" | "CHECKED_IN" | "FINISHED";

export async function getRegistrations() {
  try {
    const list = await db
      .select()
      .from(registrations)
      .orderBy(desc(registrations.createdAt));

    return { success: true, data: list };
  } catch (error) {
    console.error("Erro ao buscar inscrições:", error);
    return { success: false, data: [] };
  }
}

export async function updateRegistrationStatus(id: string, status: RegistrationStatus) {
  try {
    await db
      .update(registrations)
      .set({ status, updatedAt: new Date() })
      .where(eq(registrations.id, id));

    return { success: true };
  } catch (error) {
    console.error("Erro ao atualizar status:", error);
    return { success: false };
  }
}