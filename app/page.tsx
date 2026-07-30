import { HeroSection } from "@/components/sections/HeroSection";
import { AboutSection } from "@/components/sections/AboutSection";
import { JourneySection } from "@/components/sections/JourneySection";
import { VerseSection } from "@/components/sections/VerseSection";
import { EquipmentSection } from "@/components/sections/EquipmentSection";
import { ReflectionSection } from "@/components/sections/ReflectionSection";
import { RegistrationSection } from "@/components/sections/RegistrationSection";
import { FAQSection } from "@/components/sections/FAQSection";
import { Footer } from "@/components/layout/Footer";

export default function Home() {
  return (
    <main className="min-h-screen bg-[#08110B] text-[#F5F5F5] selection:bg-[#F38B17] selection:text-white">
      <HeroSection />
      <AboutSection />
      <JourneySection />
      <VerseSection />
      <EquipmentSection />
      <ReflectionSection />
      <RegistrationSection />
      <FAQSection />
      <Footer />
    </main>
  );
}