import { drizzle } from "drizzle-orm/postgres-js";
import postgres from "postgres";
import * as schema from "./schema";

const connectionString = process.env.DATABASE_URL!;

// Previne timeout de conexão IPv6
const client = postgres(connectionString, {
  prepare: false, // Necessário quando usa Transaction Pooler no Supabase
  connect_timeout: 10,
});

export const db = drizzle(client, { schema });