#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Pin Tanımlamaları
#define SS_PIN 10
#define RST_PIN 9
#define TRIG_PIN 7
#define ECHO_PIN 6
#define RED_LED 4      // Kırmızı LED için pin 4
#define GREEN_LED 5    // Yeşil LED için pin 5
#define BUZZER_PIN 8
#define SERVO_PIN 2

// Sabitler
#define MESAFE_ESIK 20  // cm cinsinden araç algılama mesafesi
#define BARIYER_ACIK 90
#define BARIYER_KAPALI 0
#define KART_BEKLEME_SURESI 10000  // 10 saniye

// Nesnelerin oluşturulması
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C adresi 0x27 olan 16x2 LCD
Servo bariyer;

// Yetkili kartların UID'leri
byte yetkiliKart1[4] = {0xD7, 0xF5, 0x07, 0x19};  // Coskun Irmak
byte yetkiliKart2[4] = {0xF9, 0x7C, 0x14, 0xC9};  // Murat Koca

void setup() {
  // Pin modlarının ayarlanması
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Başlangıç ayarları
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  lcd.init();
  lcd.backlight();
  bariyer.attach(SERVO_PIN);
  bariyer.write(BARIYER_KAPALI);
  
  // Hoşgeldiniz mesajı
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Otopark Sistemi");
  lcd.setCursor(0, 1);
  lcd.print("Hazir");
  delay(2000);
}

float mesafeOlc() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long sure = pulseIn(ECHO_PIN, HIGH);
  float mesafe = sure * 0.034 / 2;
  return mesafe;
}

void bariyerYavasKapat() {
  for (int pozisyon = BARIYER_ACIK; pozisyon >= BARIYER_KAPALI; pozisyon -= 2) {
    bariyer.write(pozisyon);
    delay(30);  // Daha yavaş bir iniş sağlar
  }
}

void olumluGeribildirim(String isim) {
  digitalWrite(GREEN_LED, HIGH);  // Yeşil LED açılır
  digitalWrite(RED_LED, LOW);     // Kırmızı LED kapanır
  
  // Olumlu buzzer sesi
  tone(BUZZER_PIN, 2000, 200);
  delay(200);
  tone(BUZZER_PIN, 2000, 200);
  
  // LCD mesajı
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hos Geldiniz");
  lcd.setCursor(0, 1);
  lcd.print(isim);
  
  // Bariyeri aç
  bariyer.write(BARIYER_ACIK);
  delay(5000);  // 5 saniye açık tut
  bariyerYavasKapat();
  
  digitalWrite(GREEN_LED, LOW);  // Yeşil LED kapanır
}

void olumsuzGeribildirim() {
  digitalWrite(RED_LED, HIGH);   // Kırmızı LED açılır
  digitalWrite(GREEN_LED, LOW);  // Yeşil LED kapanır
  
  // Olumsuz buzzer sesi
  tone(BUZZER_PIN, 500, 500);
  
  // LCD mesajı
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Yetkisiz Kart!");
  lcd.setCursor(0, 1);
  lcd.print("Giris Engellendi");
  
  delay(2000);
  digitalWrite(RED_LED, LOW);  // Kırmızı LED kapanır
}

bool kartKontrol(byte* okunanKart) {
  if (memcmp(okunanKart, yetkiliKart1, 4) == 0) {
    Serial.println("Kart ID: D7:F5:07:19 -> Coskun Irmak");
    olumluGeribildirim("Coskun Irmak");
    return true;
  }
  else if (memcmp(okunanKart, yetkiliKart2, 4) == 0) {
    Serial.println("Kart ID: F9:7C:14:C9 -> Murat Koca");
    olumluGeribildirim("Murat Koca");
    return true;
  }
  Serial.print("Kart ID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    if (i != rfid.uid.size - 1) {
      Serial.print(":");
    }
  }
  Serial.println(" -> Yetkisiz Kart");
  return false;
}

void loop() {
  float mesafe = mesafeOlc();
  
  if (mesafe < MESAFE_ESIK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Lutfen Kart");
    lcd.setCursor(0, 1);
    lcd.print("Okutunuz");
    
    unsigned long baslangicZamani = millis();
    
    while (millis() - baslangicZamani < KART_BEKLEME_SURESI) {
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        // Kart UID'sini Serial Monitor'e yazdır ve kontrol et
        if (!kartKontrol(rfid.uid.uidByte)) {
          olumsuzGeribildirim();
        }
        rfid.PICC_HaltA();
        break;
      }
      delay(100);
    }
    
    // Kart okutulmazsa
    if (millis() - baslangicZamani >= KART_BEKLEME_SURESI) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Zaman Asimi!");
      delay(2000);
    }
    
    // Sistem hazır mesajına dön
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Otopark Sistemi");
    lcd.setCursor(0, 1);
    lcd.print("Hazir");
  }
  
  delay(500);  // Ana döngü gecikmesi
}
