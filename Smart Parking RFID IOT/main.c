#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED (I2C)
Adafruit_SSD1306 oledIn(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SSD1306 oledOut(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RFID RC522 (SPI)
#define RST_PIN    4
#define SS_PIN_1   5  
#define SS_PIN_2   15 

MFRC522 rfidIn(SS_PIN_1, RST_PIN);
MFRC522 rfidOut(SS_PIN_2, RST_PIN);

// Pin Komponen
#define SERVO_IN_PIN   13
#define SERVO_OUT_PIN  12
#define BUZZER_IN_PIN  26
#define BUZZER_OUT_PIN 25

// Pin Ultrasonik Masuk (1)
#define TRIG_IN  14
#define ECHO_IN  27

// Pin Ultrasonik Keluar (2)
#define TRIG_OUT 32
#define ECHO_OUT 33

Servo servoIn;
Servo servoOut;

int totalSlots = 10;
int availableSlots = 10;

// Fungsi hitung jarak ultrasonik (cm)
long getDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout 30ms
  if (duration == 0) return 999; // Jika tidak ada pantulan
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);
  SPI.begin();

  rfidIn.PCD_Init();
  rfidOut.PCD_Init();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servoIn.setPeriodHertz(50);
  servoOut.setPeriodHertz(50);
  servoIn.attach(SERVO_IN_PIN, 500, 2400);
  servoOut.attach(SERVO_OUT_PIN, 500, 2400);
  
  servoIn.write(0);   
  servoOut.write(0);  

  pinMode(BUZZER_IN_PIN, OUTPUT);
  pinMode(BUZZER_OUT_PIN, OUTPUT);

  pinMode(TRIG_IN, OUTPUT);
  pinMode(ECHO_IN, INPUT);
  pinMode(TRIG_OUT, OUTPUT);
  pinMode(ECHO_OUT, INPUT);

  if(!oledIn.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("OLED Masuk Gagal!"));
  if(!oledOut.begin(SSD1306_SWITCHCAPVCC, 0x3D)) Serial.println(F("OLED Keluar Gagal!"));

  updateDisplay();
}

void loop() {
  // === GERBANG MASUK ===
  if (rfidIn.PICC_IsNewCardPresent() && rfidIn.PICC_ReadCardSerial()) {
    triggerBuzzer(BUZZER_IN_PIN);
    if (availableSlots > 0) {
      servoIn.write(90); // Buka Palang
      showMessage(oledIn, "Silahkan Masuk");
      
      // Tunggu mobil mendekat (Jarak < 10 cm) lalu lewat (Jarak > 10 cm)
      while (getDistanceCM(TRIG_IN, ECHO_IN) > 10) { delay(100); }
      while (getDistanceCM(TRIG_IN, ECHO_IN) <= 10) { delay(100); }
      
      delay(1000);
      servoIn.write(0); // Tutup Palang
      availableSlots--;
    } else {
      showMessage(oledIn, "PARKIR PENUH!");
      delay(2000);
    }
    rfidIn.PICC_HaltA();
    rfidIn.PCD_StopCrypto1();
    updateDisplay();
  }

  // === GERBANG KELUAR ===
  if (rfidOut.PICC_IsNewCardPresent() && rfidOut.PICC_ReadCardSerial()) {
    triggerBuzzer(BUZZER_OUT_PIN);
    if (availableSlots < totalSlots) {
      servoOut.write(90); // Buka Palang
      showMessage(oledOut, "Hati-hati!");
      
      // Tunggu mobil mendekat (Jarak < 10 cm) lalu lewat (Jarak > 10 cm)
      while (getDistanceCM(TRIG_OUT, ECHO_OUT) > 10) { delay(100); }
      while (getDistanceCM(TRIG_OUT, ECHO_OUT) <= 10) { delay(100); }
      
      delay(1000);
      servoOut.write(0); // Tutup Palang
      availableSlots++;
    }
    rfidOut.PICC_HaltA();
    rfidOut.PCD_StopCrypto1();
    updateDisplay();
  }
}

void triggerBuzzer(int pin) {
  digitalWrite(pin, HIGH);
  delay(120);
  digitalWrite(pin, LOW);
}

void showMessage(Adafruit_SSD1306 &display, String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(msg);
  display.display();
}

void updateDisplay() {
  oledIn.clearDisplay();
  oledIn.setTextSize(1);
  oledIn.setTextColor(SSD1306_WHITE);
  oledIn.setCursor(0, 0);
  oledIn.println("PARKIR MASUK");
  oledIn.setCursor(0, 18);
  oledIn.print("Sisa Slot: ");
  oledIn.setTextSize(2);
  oledIn.setCursor(0, 35);
  oledIn.println(availableSlots);
  oledIn.display();

  oledOut.clearDisplay();
  oledOut.setTextSize(1);
  oledOut.setTextColor(SSD1306_WHITE);
  oledOut.setCursor(0, 0);
  oledOut.println("PARKIR KELUAR");
  oledOut.setCursor(0, 18);
  oledOut.print("Sisa Slot: ");
  oledOut.setTextSize(2);
  oledOut.setCursor(0, 35);
  oledOut.println(availableSlots);
  oledOut.display();
}