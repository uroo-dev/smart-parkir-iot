#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// ==========================================
// CONFIGURATION PIN (ESP32 DEVKIT V1)
// ==========================================
#define OLED_SDA_PIN    21  // GPIO 21 (D21)
#define OLED_SCL_PIN    22  // GPIO 22 (D22)

#define IR_GATE_PIN     2   // GPIO 2  (D2)
#define BUZZER_PIN      4   // GPIO 4  (D4)
#define SERVO_PIN       13  // GPIO 13 (D13)

#define SS_PIN          5   // GPIO 5  (D5)
#define RST_PIN         17  // GPIO 17 (D17)

// ==========================================
// INSTANSIASI OBJEK
// ==========================================
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Servo palangServo;

// ==========================================
// STATE MACHINE SYSTEM
// ==========================================
enum SystemState {
  STANDBY_NO_CAR,    // Menunggu mobil mendekat
  WAITING_FOR_TAP,   // Mobil terdeteksi, menunggu tap RFID
  GATE_OPEN,         // Tap sukses, palang terbuka, mobil melintas
  CLOSING_GATE       // Mobil lepas, menghitung delay penutupan
};

SystemState currentState = STANDBY_NO_CAR;

unsigned long waktuMulaiTimer = 0;
const unsigned long JEDA_TUTUP = 3000; // Delay 3 detik setelah mobil lewat

// Helper Functions
void renderUI(String header, String mainText, String footer);
void bunyiBuzzer(int count);

void setup() {
  Serial.begin(115200);
  Serial.println(F("--- SMART PARKING GATE (PROXIMITY FIRST FLOW) ---"));

  // Inisialisasi Wire & Pin
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  pinMode(IR_GATE_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Inisialisasi RFID & SPI
  SPI.begin();
  rfid.PCD_Init();

  // Inisialisasi OLED
  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    renderUI("SMART PARKING", "  SELAMAT DATANG  ", "STATUS: GATE READY");
  }

  // Set Servo ke Posisi Tertutup (0 Derajat)
  palangServo.attach(SERVO_PIN);
  palangServo.write(0);
}

void loop() {
  // Pembacaan Sensor IR (Active LOW: LOW = Ada Mobil, HIGH = Tidak Ada Mobil)
  int statusIR = digitalRead(IR_GATE_PIN);

  switch (currentState) {
    
    // ----------------------------------------------------
    // STATE 1: MENUNGGU MOBIL MENDEKAT
    // ----------------------------------------------------
    case STANDBY_NO_CAR:
      if (statusIR == LOW) {
        // Mobil datang & terdeteksi oleh IR
        renderUI("MOBIL TERDETEKSI", " SILAKAN TAP KARTU ", "STATUS: TAP READY");
        bunyiBuzzer(1); // Beep pendek sebagai instruksi
        currentState = WAITING_FOR_TAP;
        Serial.println(F("Mobil mendekat. Menunggu Tap Kartu RFID..."));
      }
      break;

    // ----------------------------------------------------
    // STATE 2: MOBIL ADA, MENUNGGU TAP RFID
    // ----------------------------------------------------
    case WAITING_FOR_TAP:
      // Jika mobil tiba-tiba mundur/pergi sebelum tap kartu
      if (statusIR == HIGH) {
        renderUI("SMART PARKING", "  SELAMAT DATANG  ", "STATUS: GATE READY");
        currentState = STANDBY_NO_CAR;
        Serial.println(F("Mobil meninggalkan area sebelum tap. Reset to STANDBY."));
        break;
      }

      // Pembacaan RFID
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uidKartu = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          uidKartu += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
          uidKartu += String(rfid.uid.uidByte[i], HEX);
        }
        uidKartu.toUpperCase();
        Serial.println("Tap Berhasil! UID: " + uidKartu);

        bunyiBuzzer(2); // Bip 2x
        renderUI("AKSES DITERIMA", "  SILAKAN MASUK...  ", "PALANG: OPEN [^]");
        
        palangServo.write(90); // Buka Palang Servo
        currentState = GATE_OPEN;

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      break;

    // ----------------------------------------------------
    // STATE 3: PALANG TERBUKA, MOBIL SEDANG MELINTAS
    // ----------------------------------------------------
    case GATE_OPEN:
      // Menunggu sampai bagian belakang mobil lepas dari sensor IR (IR berubah ke HIGH)
      if (statusIR == HIGH) {
        waktuMulaiTimer = millis(); // Catat waktu awal lepas dari sensor
        currentState = CLOSING_GATE;
        Serial.println(F("Mobil telah melintas. Memulai hitung mundur 3 detik..."));
      }
      break;

    // ----------------------------------------------------
    // STATE 4: MOBIL SUDAH LEPAS, DELAY 3 DETIK LALU TUTUP
    // ----------------------------------------------------
    case CLOSING_GATE:
      // Anti-tailgating: Jika ada mobil lain langsung menerobos saat delay, reset timer
      if (statusIR == LOW) {
        currentState = GATE_OPEN;
        Serial.println(F("Ada objek/kendaraan lain terdeteksi! Tunda penutupan palang."));
        break;
      }

      // Jika jeda 3 detik sudah terpenuhi
      if (millis() - waktuMulaiTimer >= JEDA_TUTUP) {
        palangServo.write(0); // Tutup Palang Servo (0 Derajat)
        
        renderUI("TERIMA KASIH", "   SELAMAT JALAN   ", "STATUS: GATE CLOSED");
        delay(1500);
        
        renderUI("SMART PARKING", "  SELAMAT DATANG  ", "STATUS: GATE READY");
        currentState = STANDBY_NO_CAR;
        Serial.println(F("Palang tertutup sempurna. Kembali ke STANDBY."));
      }
      break;
  }
}

// ==========================================
// HELPER FUNCTIONS
// ==========================================
void renderUI(String header, String mainText, String footer) {
  oled.clearDisplay();
  
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println(header);
  oled.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  oled.setCursor(0, 26);
  oled.println(mainText);

  oled.drawFastHLine(0, 48, 128, SSD1306_WHITE);
  oled.setCursor(0, 52);
  oled.println(footer);

  oled.display();
}

void bunyiBuzzer(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(80);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
  }
}