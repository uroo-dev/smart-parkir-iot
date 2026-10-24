# Smart Parkir — Sistem Pintu Masuk Parkir Otomatis

![Tampak atas area gerbang smart parking](assets/gemini-generated-image-smart-parkir.jpeg)

Dokumen ini disusun untuk dua pembaca:

| Pembaca | Fokus |
|---|---|
| **Client** | Ringkasan Eksekutif, Fitur, Limitasi, Rencana Pengembangan, Biaya |
| **Tim HRD / Manajemen** | Cara Kerja, Wiring, Instalasi, Kalibrasi, Kode |

---

## 1. Ringkasan Eksekutif

Smart Parkir adalah pintu masuk parkir yang bekerja tanpa petugas. Mobil cukup mendekat, sensor ultrasonik mendeteksinya, palang servo terbuka sendiri, dan LCD memberi sambutan. Tidak ada tombol, tidak ada tiket, tidak ada antrean.

Dibangun dengan **ESP32**, sensor **HC-SR04**, servo **SG90**, dan **LCD 16x2 I2C** — komponen murah dan mudah didapat di toko elektronik lokal. Modul ESP32 sudah punya WiFi, jadi produk ini bisa dikembangkan ke IoT tanpa board tambahan.

**Waktu respon:** di bawah 1 detik sejak deteksi.
**Ketahanan:** ~3–5 tahun, bergantung kualitas komponen dan enclosure.

---

## 2. Cara Kerja Sistem

```
  POWER ON
     │
     ▼
  Inisialisasi: Serial 115200 · GPIO18 OUT · GPIO19 IN
                Servo 0° · Wire(25,26) · LCD init + backlight
     │
     ▼
  Tampilkan "Silakan Mendekat..." (2 detik)
     │
     ▼
  ┌────────────── LOOP ──────────────┐
  │  TRIG LOW 2µs → HIGH 10µs → LOW  │
  │  pulseIn(ECHO, HIGH, 30000)      │
  │  durasi == 0 ?  ── ya ──► jarak = -1
  │        │ tidak                    │
  │        ▼                          │
  │  jarak = durasi × 0.0343 / 2      │
  │  Serial → "Jarak: X cm"          │
  │        │                          │
  │        ▼                          │
  │  jarak > 0 DAN jarak ≤ 15 cm ?    │
  │     ┌──┴──┐                      │
  │   ya      tidak                   │
  │     │       │                     │
  │     ▼       ▼                     │
  │  Servo 90° Servo 0°              │
  │  LCD:      LCD:                   │
  │  "SELAMAT    "Silakan              │
  │   DATANG"    mendekat..."         │
  │  "Silakan                          │
  │   masuk"                           │
  │     │       │                     │
  │     └──┬────┘                     │
  │        ▼                          │
  │   delay(300ms) ────────────────────┘
```

**Prinsip fisika:** kecepatan suara di udara ± 343 m/s, sehingga 1&nbsp;µs berkorespondensi 0,0343&nbsp;cm. Faktor 2 dipakai karena gelombang berjalan bolak-balik (pergi-pulang).

---

## 3. Fitur Utama

| # | Fitur | Keterangan |
|---|---|---|
| 1 | Deteksi otomatis | Tanpa chip, kartu, atau tombol tekan |
| 2 | Palang servo real-time | Bereaksi di bawah 1 detik |
| 3 | LCD 16x2 I2C | Status standby, sambutan, instruksi |
| 4 | Serial monitoring | Jarak (cm) di Serial Monitor 115200 baud |
| 5 | Fail-safe | Gagal baca → palang default **menutup** |
| 6 | Hemat energi | < 100 mA pada operasi normal |

---

## 4. Arsitektur Hardware

```
      ┌─────────────────────────────────────────────┐
      │            ESP32 Dev Board (30 GPIO)        │
      │                                             │
      │  GPIO18 ──┐          ┌── GPIO25 ── SDA      │
      │  GPIO19 ──┤          ├── GPIO26 ── SCL      │
      │  GPIO23 ──┼── PWM ───┤                       │
      └───────────┼──────────┼───────────────────────┘
                  │          │
      ┌───────────┼──────────┼──────────────┬────────────────┐
      │  HC-SR04  │          │              │  LCD 16x2 I2C  │
      │           │          │              │  (addr 0x27)    │
      │ VCC ←5V   │          │              │ VCC ←3V3       │
      │ TRIG←18   │          │              │ SDA ←25        │
      │ ECHO←19   │          │              │ SCL ←26        │
      │ GND ←GND  │          │              │ GND ←GND       │
      └───────────┘          │              └────────────────┘
                             │
                    ┌────────┴────────┐
                    │   SERVO SG90    │
                    │   VCC ← 5V ext  │
                    │   SIG ← GPIO23  │
                    │   GND ← GND     │
                    └─────────────────┘
```

Tiga jalur sinyal terpisah: sensor (input analog-digital), aktuator (PWM), dan display (I2C serial).

---

## 5. Komponen yang Dibutuhkan

| Komponen | Spesifikasi | Fungsi | Harga |
|---|---|---|---|
| ESP32 Dev Board | Dual-core 240 MHz, WiFi, 30 GPIO | Otak pemroses | Rp 45.000 |
| Sensor HC-SR04 | Jangkauan 2–400 cm, akurasi ±3 mm | Ukur jarak kendaraan | Rp 12.000 |
| Servo Motor SG90 | 9 g, torsi 1,4 kg·cm, 0–180° | Gerakkan palang | Rp 15.000 |
| LCD 16x2 I2C | 16×2 karakter, alamat 0x27, backlight | Tampilkan status | Rp 18.000 |
| Kabel jumper | 20 cm, dupont | Koneksi | Rp 12.000 |
| Breadboard 400 titik | Half-size | Prototype | Rp 18.000 |
| Kabel USB-micro | Data + power | Upload & daya | Rp 15.000 |
| **Subtotal prototipe** | | | **Rp 135.000** |

**Alasan pemilihan komponen**

| Komponen | Mengapa |
|---|---|
| ESP32 | WiFi bawaan — proyek bisa naik ke IoT tanpa board tambahan. GPIO cukup untuk seluruh kebutuhan saat ini |
| HC-SR04 | Komponen paling umum (~Rp 12.000), jangkauan 4 m, standar de facto sensor jarak entry-level |
| SG90 | Ringan dan torsinya cukup untuk palang plastik. Untuk beban lebih berat: MG90S atau MG996R |
| LCD I2C | Hanya 2 kabel (SDA/SCL), cocok untuk pesan statis yang singkat |

---

## 6. Konfigurasi Pin

| Pin ESP32 | Pin Modul | Arah | Keterangan |
|---|---|---|---|
| GPIO18 | HC-SR04 TRIG | Output | Trigger pulse 10 µs |
| GPIO19 | HC-SR04 ECHO | Input | Sinyal pantulan ultrasonik |
| GPIO23 | SG90 SIG | Output (PWM) | Sudut servo 0–180° |
| GPIO25 | LCD SDA | Bidirectional | Data I2C (bukan default 21) |
| GPIO26 | LCD SCL | Output | Clock I2C (bukan default 22) |
| 5V | SG90 VCC | Power | Daya servo |
| 5V | HC-SR04 VCC | Power | Daya sensor |
| GND | Semua GND | Ground | Ground harus menyatu |

### Catatan untuk teknisi

> **WARN — tegangan dan grounding**
>
> 1. **HC-SR04 dirancang untuk 5V.** Memberi 3,3 V boleh, tapi hasil pembacaan bisa kurang stabil. Kalau pakai 5V, pastikan level ECHO aman untuk GPIO 3,3 V — gunakan resistor divider atau level shifter.
> 2. **Servo sering menyebabkan drop tegangan.** Saat servo berputar, arus melonjak dan board bisa brownout. Solusi: catu daya 5 V terpisah untuk servo + kapasitor 470 µF paralel di sisi servo, dengan **ground bersama**.
> 3. **LCD boleh berbagi catu daya** dengan sensor karena draw-nya kecil.

---

## 7. Panduan Instalasi

### 7.1 Arduino IDE

1. Unduh [Arduino IDE 2.x](https://www.arduino.cc/en/software) dan pasang.
2. Buka **File → Preferences** (`Ctrl+Shift+,`).
3. Tempelkan pada **Additional boards manager URLs**:

   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

4. Klik **OK**.

### 7.2 Board ESP32

1. **Tools → Board → Boards Manager**
2. Cari `esp32` → install **esp32 by Espressif Systems** (versi 3.x.x)
3. Tunggu sampai selesai (± 2 menit)

### 7.3 Library

1. Buka **Library Manager** (ikon buku di sidebar)
2. Cari dan install:
   - **LiquidCrystal_I2C** — Frank de Brabander
   - **ESP32Servo** — Kevin Harrington, John K. Bennett

`Wire.h` sudah bawaan board ESP32, tidak perlu diinstal.

### 7.4 Upload

1. Hubungkan ESP32 ke komputer lewat USB.
2. Buka `smart-parkir.ino`.
3. **Tools → Board → esp32 → ESP32 Dev Module**
4. **Tools → Port** → pilih port yang sesuai:
   - Windows: `COM3`, `COM4`, …
   - Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`
   - macOS: `/dev/cu.usbserial-xxxx`
5. Klik **Upload** (`Ctrl+U`).
6. Buka **Tools → Serial Monitor**, baud rate **115200**.
7. Dekatkan objek ke sensor, amati servo dan LCD.

---

## 8. Kalibrasi

### Parameter yang dapat diubah

| Parameter | Default | Fungsi | Kapan diubah |
|---|---|---|---|
| `batasJarak` | `15` (cm) | Jarak pemicu buka palang | Terlalu cepat → naikkan; terlalu lambat → turunkan |
| `TRIG_PIN` | `18` | Pin trigger | Sesuaikan wiring aktual |
| `ECHO_PIN` | `19` | Pin echo | Sesuaikan wiring aktual |
| `SERVO_PIN` | `23` | Pin PWM servo | Sesuaikan wiring aktual |
| `SDA_PIN` | `25` | Pin I2C data | Sesuaikan wiring aktual |
| `SCL_PIN` | `26` | Pin I2C clock | Sesuaikan wiring aktual |
| `lcd(0x27, …)` | `0x27` | Alamat I2C LCD | Ganti ke `0x3F` jika layar tidak merespons |

Sudut servo saat ini ditulis langsung di `loop()` sebagai `servo.write(90)` (buka) dan `servo.write(0)` (tutup). Untuk versi berikutnya, sebaiknya diekstrak menjadi konstanta `SERVO_ANGLE_BUKA` dan `SERVO_ANGLE_TUTUP` agar mudah dikalibrasi tanpa menyentuh logika.

### Menyesuaikan `batasJarak`

```
 jarak (cm)
    │
  0   5  10  15  20  25  30
    │   │   │   ▲   │   │   │
────┼───┼───┼───┼───┼───┼───┼───
    │           │   │   │   │
    │           └─ zona trigger (≤ 15 cm)
    │               servo → 90°
    │
    └─ zona aman, palang menutup
```

| Gejala | Solusi |
|---|---|
| Mobil berhenti 1–2 m sebelum palang, tidak kebuka | Naikkan `batasJarak` ke 50–100 |
| Mobil sudah lewat, servo masih getar | Turunkan `batasJarak` ke 10 |
| Palang terbuka sebelum mobil berhenti | Turunkan `batasJarak` ke 10 |
| Palang tidak mendatar saat tertutup | Ubah sudut: `write(0)` menjadi `write(10)` atau `write(15)` |

---

## 9. Skenario Pengujian

| # | Skenario | Langkah | Hasil yang diharapkan |
|---|---|---|---|
| 1 | Deteksi normal | Tangan 10 cm di depan sensor | Serial: `Jarak: 10.00 cm` · servo 90° · LCD: `SELAMAT DATANG` / `Silakan masuk` |
| 2 | Terlalu jauh | Tangan 50 cm di depan sensor | Serial: `Jarak: 50.00 cm` · servo 0° · LCD: `Silakan mendekat...` |
| 3 | Timeout | Sensor terhalang rapat | Serial: `Tidak terbaca` · servo 0° (tetap menutup) |
| 4 | Inisialisasi | Tekan reset | LCD: `Silakan` / `Mendekat...` selama 2 detik |
| 5 | Stabilitas | Biarkan menyala 1 menit | Tidak hang, tidak reset, servo & LCD konsisten |
| 6 | Blind zone | Objek < 2 cm dari sensor | Deteksi tidak valid · palang menutup (aman) |

---

## 10. Pembacaan Kode

File: `smart-parkir.ino` (124 baris)

### Konstanta global

```cpp
#define TRIG_PIN 18
#define ECHO_PIN 19
#define SERVO_PIN 23

// LCD kamu diubah dari 21/22 menjadi 25/26
#define SDA_PIN 25
#define SCL_PIN 26

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

const int batasJarak = 15;
```

### `setup()` — Inisialisasi

Menyiapkan Serial Monitor, pin sensor, servo pada posisi tertutup, I2C, lalu LCD menampilkan ajakan mendekat selama 2 detik sebagai penanda sistem sudah siap.

### `bacaJarak()` — Fungsi pengukuran

```cpp
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
```

Nilai balik `-1` berarti gagal mengukur. Timeout 30&nbsp;000&nbsp;µs (≈ 5 m) mencegah program hang pada sensor yang bermasalah.

### `loop()` — Logika utama

```cpp
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
```

Syarat `jarak > 0` mencegah nilai `-1` (gagal baca) ikut memicu palang.

---

## 11. Limitasi yang Perlu Diketahui

| # | Limitasi | Dampak | Mitigasi |
|---|---|---|---|
| 1 | Tidak ada autentikasi | Mobil apa pun bisa masuk | Tambah RFID, QR gate, atau kamera |
| 2 | Tidak ada hitungan slot | Kapasitas tersisa tidak diketahui | Tambah sensor ultrasonik per slot |
| 3 | HC-SR04 buta < 2 cm | Objek sangat dekat tidak terbaca | Sesuaikan posisi sensor |
| 4 | Tidak tahan air | Elektronik rusak jika outdoor | Gunakan enclosure IP65 |
| 5 | Tidak ada log/history | Tidak ada jejak audit | Tambah RTC + logging ke SD card |
| 6 | `lcd.clear()` dipanggil tiap 300 ms | Layar LCD berkedip setiap siklus | Gunakan `lcd.setCursor()` tanpa `clear()`, atau tulis pesan hanya saat isinya berubah |
| 7 | `pulseIn()` memblokir | Maksimal ~30 ms per pengukuran | Tidak masalah pada aplikasi ini |

---

## 12. Rencana Pengembangan

```
v1.0  (saat ini)
 ├── ✅ Deteksi ultrasonik
 ├── ✅ Kendali servo
 ├── ✅ Display LCD
 └── ✅ Logging serial

v1.1  (perbaikan teknis)
 ├── 🔧 Power servo eksternal + kapasitor 470 µF
 ├── 🔧 Level shifter 5V → 3,3V untuk pin ECHO
 ├── 🔧 Hilangkan kedipan LCD
 └── 🔧 Ekstrak konstanta sudut servo

v2.0  (ekspansi fungsional)
 ├── 🔐 RFID untuk akses terotorisasi
 ├── 📊 Counter slot parkir per bay
 └── 📱 Dashboard web monitoring

v3.0  (enterprise)
 ├── ☁️ Integrasi cloud (Blynk / Firebase / ThingSpeak)
 ├── 💳 Pembayaran otomatis (QRIS / barcode)
 ├── 📄 Laporan PDF harian
 └── 🔔 Notifikasi real-time via Telegram
```

---

## 13. Ringkasan Biaya

| Kategori | Komponen | Estimasi |
|---|---|---|
| Mikrokontroler | ESP32 Dev Board | Rp 45.000 |
| Sensor | HC-SR04 | Rp 12.000 |
| Aktuator | Servo SG90 | Rp 15.000 |
| Display | LCD 16x2 I2C | Rp 18.000 |
| Aksesori | Jumper, breadboard, kabel USB | Rp 45.000 |
| **Subtotal prototipe** | | **Rp 135.000** |
| Instalasi | Enclosure, catu daya 5 V, level shifter, kapasitor, label | Rp 65.000 |
| **Total instalasi** | | **Rp 200.000** |

Biaya bahan 연간 (pemeliharaan): ± Rp 25.000 untuk penggantian sensor dan servo yang aus.

---

## Ringkasan Singkat

| Pertanyaan | Jawaban |
|---|---|
| Apa ini? | Pintu masuk parkir otomatis berbasis ESP32 |
| Cara kerjanya? | Sensor ultrasonik mendeteksi → palang servo terbuka otomatis |
| Berapa biayanya? | ± Rp 200.000 termasuk instalasi |
| Berapa lama umur pakainya? | 3–5 tahun, bergantung kualitas komponen dan enclosure |
| Siapa yang cocok? | Perkantoran, mall, perumahan, fasilitas umum |
| Apa saja pengembangan lanjutannya? | RFID, counter slot, dashboard IoT, pembayaran otomatis |

---

## Asset Gambar

| Keterangan | Nilai |
|---|---|
| Sumber | Image 1 — referensi visual konsep Smart Parkir |
| Nama file | `Gemini_Generated_Image_rc4ub8rc4ub8rc4u.jpeg` |
| Lokasi di dokumen | Bagian atas halaman (hero image) |
| Status | **Placeholder** — file gambar belum ada di repository |

**Langkah memasang aset:**

1. Simpan gambar ke `assets/gemini-generated-image-smart-parkir.jpeg`
2. Ganti nama file bila berbeda, lalu sesuaikan path di baris paling atas dokumen
3. Hapus bagian ini setelah aset terpasang

Caption yang disarankan saat gambar sudah terpasang:

> *Tampak atas area gerbang: mobil berhenti sekitar 15 cm dari palang, sensor HC-SR04 mengukur jarak, servo mengangkat palang, dan LCD menampilkan "SELAMAT DATANG".*
