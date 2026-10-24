# 🔌 Panduan Pengkabelan — Smart Parkir ESP32

Panduan lengkap memasang semua komponen pada **ESP32 NodeMCU-32S 30-pin** yang terpasang pada **ESP32 Expansion Board 30P**.

## Header Expansion Board (G-V-S)

| Baris | Arti | Penggunaan |
| :-: | --- | --- |
| G (dekat ESP32) | GND | semua return |
| V (tengah) | mengikuti jumper selektor | set 5V untuk servo dan IR |
| S (paling luar) | sinyal GPIO | semua pin kontrol |

> Blok header 2x4 (VCC/3V/GND) dipakai untuk mengambil pin 3V — suplai RC522 dan OLED dari 3V, bukan dari baris V, agar beban servo tidak memengaruhi keduanya.

## 1. Dua modul RC522 (SPI dibagi)

Empat kolom breadboard — satu kolom per sinyal, **kedua** RC522 diparalel di kolom yang sama:

| Kolom | Sinyal | GPIO |
| :-: | --- | :-: |
| 1 | SCK | D18 |
| 2 | MOSI | D23 |
| 3 | MISO | D19 |
| 4 | RST | D22 |

- SS (SDA / chip select) terpisah per modul: RFID masuk = D5, RFID keluar = D4.
- VCC kedua modul = pin 3V; GND = baris G.
- IRQ dikosongkan (tidak dipakai).
- Urutan pin fisik RC522: SDA, SCK, MOSI, MISO, IRQ, GND, RST, 3.3V.

> Chip kloning v0x82 tetap berfungsi. Nilai 0x00/0xFF artinya jalur SPI atau daya bermasalah.

## 2. Dua OLED 0.96 inch (dua bus I2C)

| OLED | Bus | SDA | SCL | Alamat |
| --- | --- | :-: | :-: | --- |
| Pintu masuk | Wire | D26 | D32 | 0x3C (fallback 0x3D) |
| Pintu keluar | Wire1 | D21 | D33 | 0x3C (fallback 0x3D) |

- VCC = 3V; GND = baris G.
- Peringatan: `Adafruit_SSD1306::begin()` bisa mengembalikan `true` walau modul tidak merespons — gunakan sketch diagnosa (`endTransmission`) untuk konfirmasi nyata.

## 3. Servo, IR, Buzzer

| Komponen | Pin sinyal | Catatan |
| --- | :-: | --- |
| Servo palang masuk | D12 | GPIO12 = strapping pin MTDI, hati-hati saat boot |
| Servo palang keluar | D14 | sinyal biasa |
| Sensor IR | D13 | LOW saat ada objek; daya ke rail 5V |
| Buzzer aktif | D27 | aktif HIGH |

Servo dan IR ke baris V (5V) + G; jalur sinyal ke baris S.

## 4. Checklist Pasang

- Kedua RC522: 4 jalur bus di kolom yang sama, VCC 3V, GND baris G
- SS tiap RC522 ke D5 dan D4 (terpisah per gate)
- OLED masuk D26/D32; OLED keluar D21/D33 (jangan tertukar)
- IR dan servo di rail 5V; jumper selektor expansion = 5V
- Buzzer D27; semua GND digabung
- Flash `smart_parking_diag/` dulu — pastikan [SCAN] semua OK, baru pakai firmware produksi
