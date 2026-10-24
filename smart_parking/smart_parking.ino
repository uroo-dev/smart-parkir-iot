#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// PIN RFID RC522
#define RST_PIN          22
#define SS_1_PIN         5
#define SS_2_PIN         4

// PIN KOMPONEN LAIN
#define OLED1_SDA        26   // OLED PINTU MASUK
#define OLED1_SCL        32
#define OLED2_SDA        21   // OLED PINTU KELUAR (bus I2C kedua = Wire1)
#define OLED2_SCL        33
#define IR_SENSOR_PIN    13
#define SERVO_MASUK_PIN  12
#define SERVO_KELUAR_PIN 14
#define BUZZER_PIN       27

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ALAMAT_1 0x3C
#define OLED_ALAMAT_2 0x3D

MFRC522 rfidMasuk(SS_1_PIN, RST_PIN);
MFRC522 rfidKeluar(SS_2_PIN, RST_PIN);

Servo servoMasuk;
Servo servoKeluar;

Adafruit_SSD1306 oledMasuk(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SSD1306 oledKeluar(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);

bool buzzerSudahBunyi  = false;
bool oledMasukSiap     = false;
bool oledKeluarSiap    = false;
int  statusIrTerakhir  = HIGH;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("=== SMART PARKING v4 (2 OLED: MASUK + KELUAR) ==="));

  SPI.begin();

  rfidMasuk.PCD_Init();
  rfidKeluar.PCD_Init();
  Serial.println(F("[RFID] PCD_Init selesai (keduanya)"));

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  servoMasuk.attach(SERVO_MASUK_PIN);
  servoKeluar.attach(SERVO_KELUAR_PIN);
  servoMasuk.write(0);
  servoKeluar.write(0);

  Serial.println(F("[SELF-TEST] Buzzer 2x"));
  beep(120);
  delay(120);
  beep(120);

  Serial.println(F("[SERVO1] Gerakan masuk 0->90->0 (amati servo 1)"));
  servoMasuk.write(90);
  delay(800);
  servoMasuk.write(0);
  delay(600);

  Serial.println(F("[SERVO2] Gerakan keluar 0->90->0 (amati servo 2)"));
  servoKeluar.write(90);
  delay(800);
  servoKeluar.write(0);
  delay(600);

  // OLED PINTU MASUK (Wire, D26/D32)
  Wire.begin(OLED1_SDA, OLED1_SCL);
  oledMasukSiap = oledMasuk.begin(SSD1306_SWITCHCAPVCC, OLED_ALAMAT_1);
  if (!oledMasukSiap) oledMasukSiap = oledMasuk.begin(SSD1306_SWITCHCAPVCC, OLED_ALAMAT_2);
  Serial.printf("[OLED-MASUK] Status: %s (SDA=%d, SCL=%d)\n", oledMasukSiap ? "OK" : "GAGAL", OLED1_SDA, OLED1_SCL);

  // OLED PINTU KELUAR (Wire1, D21/D33)
  Wire1.begin(OLED2_SDA, OLED2_SCL);
  oledKeluarSiap = oledKeluar.begin(SSD1306_SWITCHCAPVCC, OLED_ALAMAT_1);
  if (!oledKeluarSiap) oledKeluarSiap = oledKeluar.begin(SSD1306_SWITCHCAPVCC, OLED_ALAMAT_2);
  Serial.printf("[OLED-KELUAR] Status: %s (SDA=%d, SCL=%d)\n", oledKeluarSiap ? "OK" : "GAGAL", OLED2_SDA, OLED2_SCL);

  uint8_t versiRfidMasuk  = rfidMasuk.PCD_ReadRegister(MFRC522::VersionReg);
  uint8_t versiRfidKeluar = rfidKeluar.PCD_ReadRegister(MFRC522::VersionReg);
  laporkanRfid("RFID MASUK (D5)", versiRfidMasuk);
  laporkanRfid("RFID KELUAR (D4)", versiRfidKeluar);

  Serial.printf("[IR] Nilai pin D13 sekarang: %d (LOW = ada objek)\n", digitalRead(IR_SENSOR_PIN));

  Serial.println(F("=== DIAGNOSA SELESAI, SISTEM JALAN ==="));
  if (oledMasukSiap) tampilkanStandby(oledMasuk, true);
  if (oledKeluarSiap) tampilkanStandby(oledKeluar, false);
}

void loop() {
  int irStatus = digitalRead(IR_SENSOR_PIN);
  if (irStatus != statusIrTerakhir) {
    statusIrTerakhir = irStatus;
    Serial.printf("[IR] Perubahan: %d (LOW = ada objek)\n", irStatus);
  }

  // 1. LOGIKA PINTU MASUK (IR ACTIVE LOW, wajib ada objek dulu)
  if (irStatus == LOW) {
    if (!buzzerSudahBunyi) {
      beep(120);
      buzzerSudahBunyi = true;
    }

    if (oledMasukSiap) tampilkanAnimasiTap();

    if (rfidMasuk.PICC_IsNewCardPresent() && rfidMasuk.PICC_ReadCardSerial()) {
      Serial.print(F("[TAP MASUK] UID = "));
      cetakUID(rfidMasuk.uid);
      beep(100);

      if (oledMasukSiap) tampilkanInfo(oledMasuk, F("AKSES DITERIMA!"), F("Silahkan Masuk..."));

      servoMasuk.write(90);

      while (digitalRead(IR_SENSOR_PIN) == LOW) {
        delay(100);
      }
      delay(2000);
      servoMasuk.write(0);

      rfidMasuk.PICC_HaltA();
      rfidMasuk.PCD_StopCrypto1();
      buzzerSudahBunyi = false;
      if (oledMasukSiap) tampilkanStandby(oledMasuk, true);
    }
  } else {
    if (buzzerSudahBunyi) {
      buzzerSudahBunyi = false;
      if (oledMasukSiap) tampilkanStandby(oledMasuk, true);
    }
  }

  // 2. LOGIKA PINTU KELUAR (LANGSUNG TAP, TANPA SENSOR IR)
  if (rfidKeluar.PICC_IsNewCardPresent() && rfidKeluar.PICC_ReadCardSerial()) {
    Serial.print(F("[TAP KELUAR] UID = "));
    cetakUID(rfidKeluar.uid);
    beep(100);

    if (oledKeluarSiap) tampilkanInfo(oledKeluar, F("AKSES KELUAR"), F("Hati-hati di Jalan!"));

    servoKeluar.write(90);
    delay(5000);
    servoKeluar.write(0);

    rfidKeluar.PICC_HaltA();
    rfidKeluar.PCD_StopCrypto1();
    if (oledKeluarSiap) tampilkanStandby(oledKeluar, false);
  }

  delay(30);
}

// ==========================================
// FUNGSI BANTU
// ==========================================

void beep(int durasi) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durasi);
  digitalWrite(BUZZER_PIN, LOW);
}

void laporkanRfid(const char *nama, uint8_t versi) {
  if (versi == 0x00 || versi == 0xFF) {
    Serial.printf("[RFID] %s: MASALAH! chip v%.2X (00/FF = cek jalur SPI & daya 3.3V)\n", nama, versi);
  } else {
    Serial.printf("[RFID] %s: OK (chip v%.2X — kloning/versi lain tetap jalan)\n", nama, versi);
  }
}

void cetakUID(MFRC522::Uid uid) {
  for (byte i = 0; i < uid.size; i++) {
    Serial.printf("%02X ", uid.uidByte[i]);
  }
  Serial.println();
}

// masuk = true  -> layar "Selamat Datang" besar
// masuk = false -> layar identitas pintu keluar
void tampilkanStandby(Adafruit_SSD1306 &d, bool masuk) {
  d.clearDisplay();
  d.setTextSize(2);
  d.setTextColor(WHITE);

  if (masuk) {
    d.setCursor(10, 8);
    d.println(F("Selamat"));
    d.setCursor(28, 28);
    d.println(F("Datang"));
    d.setTextSize(1);
    d.setCursor(46, 50);
    d.println("(^_^)");
  } else {
    d.setCursor(24, 8);
    d.println(F("PINTU"));
    d.setCursor(24, 28);
    d.println(F("KELUAR"));
    d.setTextSize(1);
    d.setCursor(0, 50);
    d.println(F("Tap Kartu Untuk Keluar"));
  }
  d.display();
}

// Animasi tap kartu (hanya dipakai di OLED pintu masuk)
void tampilkanAnimasiTap() {
  static unsigned long lastFrame = 0;
  static int frame = 0;

  if (millis() - lastFrame > 300) {
    lastFrame = millis();
    frame = (frame + 1) % 3;

    oledMasuk.clearDisplay();
    oledMasuk.setTextSize(1);
    oledMasuk.setTextColor(WHITE);
    oledMasuk.setCursor(8, 5);
    oledMasuk.println(F("Silahkan Tap Kartu"));

    oledMasuk.drawRect(85, 25, 30, 35, WHITE);
    oledMasuk.drawRect(88, 28, 24, 29, WHITE);

    int posX = 15 + (frame * 20);
    oledMasuk.fillRect(posX, 30, 22, 16, WHITE);
    oledMasuk.drawPixel(posX + 4, 34, BLACK);

    oledMasuk.display();
  }
}

void tampilkanInfo(Adafruit_SSD1306 &d, const __FlashStringHelper *baris1, const __FlashStringHelper *baris2) {
  d.clearDisplay();
  d.setTextSize(1);
  d.setTextColor(WHITE);
  d.setCursor(15, 15);
  d.println(baris1);
  d.setCursor(10, 35);
  d.println(baris2);
  d.display();
}
