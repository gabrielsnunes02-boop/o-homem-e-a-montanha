import type { Metadata } from "next";
import { Inter, Bebas_Neue } from "next/font/google";
import "./globals.css";

const inter = Inter({
  variable: "--font-inter-sans",
  subsets: ["latin"],
});

const bebasNeue = Bebas_Neue({
  weight: "400",
  variable: "--font-bebas-neue",
  subsets: ["latin"],
});

export const metadata: Metadata = {
  title: "O Homem e a Montanha | O desafio que te transforma",
  description: "Uma jornada de coragem, fé e transformação espiritual promovida pela Igreja Viva em Missões.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="pt-BR" className={`${inter.variable} ${bebasNeue.variable}`}>
      <body className="antialiased bg-bg-primary text-text-white min-h-screen">
        {children}
      </body>
    </html>
  );
}