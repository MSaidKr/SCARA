#include <Stepper.h>

// --- AYARLAR ---
const int STEPS_PER_REV = 2048; 
const int HASSASIYET = 50; // Her tuşa bastığında kaç adım atsın?
// Not: Bu sayıyı arttırırsan (örn: 200) tuşa bir kere basınca çok döner.
// Azaltırsan (örn: 10) daha hassas milim milim kontrol edersin.

// --- MOTOR TANIMLAMALARI ---
// Motor 1 (Gövde) -> Pin 8, 10, 9, 11
Stepper motor1(STEPS_PER_REV, 8, 10, 9, 11); 

// Motor 2 (Dirsek) -> Pin 4, 6, 5, 7
Stepper motor2(STEPS_PER_REV, 4, 6, 5, 7); 

// Motor 3 (Kıskaç/Z) -> Pin A0, A2, A1, A3
Stepper motor3(STEPS_PER_REV, A0, A2, A1, A3); 

void setup() {
  Serial.begin(9600); // Seri haberleşmeyi başlat
  
  // HIZ AYARI 🐢
  // Güçlü olması için hızı düşük tutuyoruz.
  // İleride alıştıkça 10 veya 15 yapabilirsin.
  motor1.setSpeed(7); 
  motor2.setSpeed(7);
  motor3.setSpeed(7);
  
  Serial.println("--- ROBOT KOL KONTROL PANELI ---");
  Serial.println("Gorev: Q-W (Govde) | A-S (Dirsek) | Z-X (Kiskac)");
  Serial.println("Lutfen harfleri kucuk harf olarak girin.");
}

void loop() {
  // Bilgisayardan veri geliyor mu diye dinle
  if (Serial.available() > 0) {
    
    // Gelen harfi oku
    char tus = Serial.read();

    // --- MOTOR 1 (GÖVDE) ---
    if (tus == 'q') {
      Serial.println("Govde: Sola Donuyor <--");
      motor1.step(HASSASIYET); 
    }
    else if (tus == 'w') {
      Serial.println("Govde: Saga Donuyor -->");
      motor1.step(-HASSASIYET); 
    }
    
    // --- MOTOR 2 (DİRSEK) ---
    else if (tus == 'a') {
      Serial.println("Dirsek: Yukari/Sola <--");
      motor2.step(HASSASIYET); 
    }
    else if (tus == 's') {
      Serial.println("Dirsek: Asagi/Saga -->");
      motor2.step(-HASSASIYET); 
    }

    // --- MOTOR 3 (KISKAÇ / Z-EKSEN) ---
    else if (tus == 'z') {
      Serial.println("Kiskac: Aciliyor/Yukari");
      motor3.step(HASSASIYET); 
    }
    else if (tus == 'x') {
      Serial.println("Kiskac: Kapaniyor/Asagi");
      motor3.step(-HASSASIYET); 
    }
    
    // Motorlar durduğunda bobinleri serbest bırak (Opsiyonel)
    // Eğer motorlar çok ısınıyorsa veya güce ihtiyaç yoksa bu satırları açabilirsin.
    // digitalWrite(8, LOW); digitalWrite(9, LOW); ... (gibi)
  }
}
