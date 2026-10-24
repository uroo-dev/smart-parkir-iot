#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define TRIG_PIN 18
#define ECHO_PIN 19
#define SERVO_PIN 23

// LCD kamu diubah dari 21/22 menjadi 25/26
#define SDA_PIN 25
#define SCL_PIN 26

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servo;

const int batasJarak = 15;

void setup()
{
    Serial.begin(115200);

    // Ultrasonic
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Servo
    servo.attach(SERVO_PIN);
    servo.write(0);

    // LCD I2C
    Wire.begin(SDA_PIN, SCL_PIN);

    lcd.init();
    lcd.backlight();

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Silakan");

    lcd.setCursor(0, 1);
    lcd.print("Mendekat...");

    delay(2000);

    lcd.clear();
}

float bacaJarak()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    long durasi = pulseIn(
        ECHO_PIN,
        HIGH,
        30000
    );

    if (durasi == 0)
    {
        return -1;
    }

    float jarak =
        durasi * 0.0343 / 2;

    return jarak;
}

void loop()
{
    float jarak = bacaJarak();

    Serial.print("Jarak: ");

    if (jarak < 0)
    {
        Serial.println("Tidak terbaca");
    }
    else
    {
        Serial.print(jarak);
        Serial.println(" cm");
    }

    // Jika ada benda <= 15 cm
    if (jarak > 0 && jarak <= batasJarak)
    {
        servo.write(90);

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("SELAMAT DATANG");

        lcd.setCursor(0, 1);
        lcd.print("Silakan masuk");

        delay(300);
    }

    // Jika tidak ada benda
    else
    {
        servo.write(0);

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("Silakan");

        lcd.setCursor(0, 1);
        lcd.print("mendekat...");

        delay(300);
    }
}