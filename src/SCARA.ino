#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Stepper.h>

// ==========================================
//  1. KULLANICI AYARLARI
// ==========================================
const int adimSayisi = 2048; // 28BYJ-48 için tam tur adımı
const int motorHizi = 12;    // İdeal RPM (10-15)
const int ESP32_BEKLEME_SURESI = 15000; // Hedef bulununca ESP32'nin açık kalma süresi (ms)

// ==========================================
//  2. DONANIM PİNLERİ
// ==========================================
#define TRIG_PIN 2
#define ECHO_PIN 3
#define MOSFET_PIN 4
#define DC_MOTOR_ROLE 13 // 5V Röle Pini

// Stepper Motor Faz Sırası: 1-3-2-4
Stepper omuzMotor(adimSayisi, 9, 11, 10, 12); 
Stepper dirsekMotor(adimSayisi, 5, 7, 6, 8);
Stepper zEksenMotor(adimSayisi, A0, A2, A1, A3); // Uç Motor (Manipülatör)

Adafruit_INA219 ina219;

// Sistem Durum Değişkenleri
bool hedefeKilitlendi = false;
unsigned long esp32AcilisZamani = 0;

void setup() {
  Serial.begin(115200); 
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOSFET_PIN, OUTPUT);
  pinMode(DC_MOTOR_ROLE, OUTPUT);
  
  digitalWrite(DC_MOTOR_ROLE, LOW); // Başlangıçta DC motor kesinlikle dursun
  digitalWrite(MOSFET_PIN, LOW);    // Başlangıçta ESP32 Kapalı
  
  if (!ina219.begin()) {
    Serial.println(F("CRITICAL ERROR: INA219 Bulunamadi!"));
    while (1);
  }

  omuzMotor.setSpeed(motorHizi);
  dirsekMotor.setSpeed(motorHizi);
  zEksenMotor.setSpeed(motorHizi);

  Serial.println(F("SCARA Sistemi Hazir."));
  akimOku();
  Serial.println(F("Uyku Modu: El İle Uyandırma Bekleniyor..."));
}

void loop() {
  
  // --- AŞAMA 1: UYKU MODU VE EL İLE UYANDIRMA ---
  if (!hedefeKilitlendi) {
    int el_mesafesi = mesafeOlc();
    
    // Eğer sensörün 2 cm ile 15 cm yakınına el sallanırsa
    if (el_mesafesi > 2 && el_mesafesi < 15) {
      Serial.println(F(">>> EL SALLANDI! SİSTEM UYANIYOR <<<"));
      
      digitalWrite(MOSFET_PIN, HIGH); // MOSFET'i aç, kameralara ve sürücülere güç ver
      delay(500); 
      akimOku(); 
      
      hedefeKilitlendi = true;
      esp32AcilisZamani = millis();
      
      Serial.println(F("Kamera Isiniyor... Python Emri Bekleniyor..."));
      delay(3000); // Ağ bağlantısı için avans
    }
    delay(500); // Sensörü yormamak için ölçüm aralığı
  } 
  
  // --- AŞAMA 2: VERİ BEKLEME VE HAREKET MODU ---
  else {
    
    // Python'dan Wi-Fi -> ESP32 -> Kablo üzerinden veri gelirse
    if (Serial.available() > 0) {
        String gelenVeri = Serial.readStringUntil('\n'); // Örn: O500D200
        
        int oIndeks = gelenVeri.indexOf('O');
        int dIndeks = gelenVeri.indexOf('D');
        
        // Veri formata uygunsa adımları ayıkla
        if (oIndeks != -1 && dIndeks != -1) {
            int omuz_hedef = gelenVeri.substring(oIndeks + 1, dIndeks).toInt();
            int dirsek_hedef = gelenVeri.substring(dIndeks + 1).toInt();
            
            Serial.print(F("Hareket Ediliyor -> Omuz: ")); Serial.print(omuz_hedef);
            Serial.print(F(" Dirsek: ")); Serial.println(dirsek_hedef);

            // 1. Kollar Hedefe Gider
            omuzMotor.step(omuz_hedef);
            dirsekMotor.step(dirsek_hedef);
            
            // 2. Kollar hedefe vardıktan sonra Manipülatör (Z-Ekseni) ve DC Motor çalışır
            hedefiAl(); 
        }
    }

    // Görev zaman aşımı (Kamera açık kalma süresi dolduysa sistemi uyut)
    if (millis() - esp32AcilisZamani > ESP32_BEKLEME_SURESI) {
        Serial.println(F("Gorev Tamam. Sistem Uyku Moduna Aliniyor..."));
        digitalWrite(MOSFET_PIN, LOW); // Gücü kes, donanımı koru
        hedefeKilitlendi = false;      
        
        Serial.println(F("Uyku Oncesi Son Guc Raporu:"));
        akimOku();
    }
  }
}

// ==========================================
//  YARDIMCI FONKSİYONLAR
// ==========================================

int mesafeOlc() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long sure = pulseIn(ECHO_PIN, HIGH);
  return sure * 0.034 / 2;
}

void hedefiAl() {
    const int manipulatorAdim = 1479; 
    
    Serial.println(F("Z-Ekseni Asagi Iniyor..."));
    zEksenMotor.step(manipulatorAdim); 
    
    Serial.println(F("DC Motor Calistirildi (Role Cekti)"));
    digitalWrite(DC_MOTOR_ROLE, HIGH); 
    
    akimOku(); // DC motor devredeyken akım kontrolü
    delay(1500); // DC motorun çalışma süresi
    
    digitalWrite(DC_MOTOR_ROLE, LOW);
    Serial.println(F("DC Motor Durduruldu."));
    
    Serial.println(F("Z-Ekseni Tekrar Yukari Geri Donuyor..."));
    zEksenMotor.step(-manipulatorAdim); 
    delay(1000); 
}

void akimOku() {
  float ma = ina219.getCurrent_mA();
  float v = ina219.getBusVoltage_V();
  Serial.print(F("[INA219 RAPOR] Voltaj: ")); Serial.print(v);
  Serial.print(F(" V | Cekilen Akim: ")); Serial.print(ma);
  Serial.println(F(" mA"));
}
