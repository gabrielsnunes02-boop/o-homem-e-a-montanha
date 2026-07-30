import {
  pgTable,
  uuid,
  text,
  timestamp,
  integer,
  boolean,
  numeric,
  pgEnum,
  index,
} from "drizzle-orm/pg-core";

// Enums do Status de Inscrição e Pagamento
export const registrationStatusEnum = pgEnum("registration_status", [
  "PENDING",
  "CONFIRMED",
  "WAITING_LIST",
  "CHECKED_IN",
  "FINISHED",
  "CANCELLED",
]);

export const paymentStatusEnum = pgEnum("payment_status", [
  "PENDING",
  "PAID",
  "FAILED",
  "EXPIRED",
  "REFUNDED",
  "CANCELLED",
]);

export const paymentMethodEnum = pgEnum("payment_method", [
  "PIX",
  "CREDIT_CARD",
]);

// 1. Tabela: churches
export const churches = pgTable("churches", {
  id: uuid("id").primaryKey().defaultRandom(),
  name: text("name").notNull(),
  city: text("city").notNull(),
  state: text("state").notNull(),
  logoUrl: text("logo_url"),
  instagram: text("instagram"),
  createdAt: timestamp("created_at").defaultNow().notNull(),
  updatedAt: timestamp("updated_at").defaultNow().notNull(),
});

// 2. Tabela: events
export const events = pgTable(
  "events",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    churchId: uuid("church_id")
      .references(() => churches.id)
      .notNull(),
    title: text("title").notNull(),
    slug: text("slug").notNull().unique(),
    theme: text("theme").notNull(),
    verse: text("verse").notNull(),
    description: text("description"),
    location: text("location").notNull(),
    meetingPoint: text("meeting_point").notNull(),
    coverImage: text("cover_image"),
    eventDate: timestamp("event_date").notNull(),
    startTime: text("start_time").notNull(),
    endTime: text("end_time"),
    registrationDeadline: timestamp("registration_deadline"),
    minAge: integer("min_age").default(18),
    maxParticipants: integer("max_participants").notNull(),
    registrationFee: numeric("registration_fee", { precision: 10, scale: 2 }).notNull(),
    whatsappGroup: text("whatsapp_group"),
    registrationOpen: boolean("registration_open").default(true).notNull(),
    createdAt: timestamp("created_at").defaultNow().notNull(),
    updatedAt: timestamp("updated_at").defaultNow().notNull(),
  },
  (table) => [
    index("events_slug_idx").on(table.slug),
    index("events_date_idx").on(table.eventDate),
  ]
);

// 3. Tabela: registrations
// 3. Tabela: registrations
// Em lib/db/schema.ts
export const registrations = pgTable(
  "registrations",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    registrationCode: text("registration_code").notNull().unique(),
    
    // TEM QUE SER TEXT SIMPLES (Sem .references e sem uuid())
    eventId: text("event_id").notNull().default("event-01"),
    
    fullName: text("full_name").notNull(),
    phone: text("phone").notNull(),
    email: text("email"),
    birthDate: timestamp("birth_date").notNull(),
    church: text("church"),
    city: text("city"),
    emergencyName: text("emergency_name").notNull(),
    emergencyPhone: text("emergency_phone").notNull(),
    medicalRestrictions: text("medical_restrictions"),
    acceptedTerms: boolean("accepted_terms").notNull().default(false),
    status: registrationStatusEnum("status").default("PENDING").notNull(),
    createdAt: timestamp("created_at").defaultNow().notNull(),
    updatedAt: timestamp("updated_at").defaultNow().notNull(),
  },
  (table) => [
    index("registrations_event_id_idx").on(table.eventId),
    index("registrations_phone_idx").on(table.phone),
    index("registrations_status_idx").on(table.status),
  ]
);

// 4. Tabela: payments
export const payments = pgTable(
  "payments",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    registrationId: uuid("registration_id")
      .references(() => registrations.id)
      .notNull(),
    provider: text("provider").notNull(),
    providerPaymentId: text("provider_payment_id"),
    paymentMethod: paymentMethodEnum("payment_method").notNull(),
    amount: numeric("amount", { precision: 10, scale: 2 }).notNull(),
    currency: text("currency").default("BRL").notNull(),
    status: paymentStatusEnum("status").default("PENDING").notNull(),
    pixCode: text("pix_code"),
    pixQrcode: text("pix_qrcode"),
    expiresAt: timestamp("expires_at"),
    paidAt: timestamp("paid_at"),
    createdAt: timestamp("created_at").defaultNow().notNull(),
    updatedAt: timestamp("updated_at").defaultNow().notNull(),
  },
  (table) => [
    index("payments_registration_id_idx").on(table.registrationId),
    index("payments_provider_id_idx").on(table.providerPaymentId),
    index("payments_status_idx").on(table.status),
  ]
);

// 5. Tabela: checkins
export const checkins = pgTable("checkins", {
  id: uuid("id").primaryKey().defaultRandom(),
  registrationId: uuid("registration_id")
    .references(() => registrations.id)
    .notNull(),
  checkedAt: timestamp("checked_at").defaultNow().notNull(),
  checkedBy: text("checked_by"),
  notes: text("notes"),
  createdAt: timestamp("created_at").defaultNow().notNull(),
});