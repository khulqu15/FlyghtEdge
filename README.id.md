<div align="center">

# FlyghtNinno

**Dashboard web di ponsel untuk semua ESP32, cukup 5 baris kode.**

Portal setup Wi-Fi · captive portal yang terbuka sendiri · mDNS · login & sesi · dashboard drag-and-drop ·
lampu & efek · alur visual (flows) · MQTT + Home Assistant · HTTP REST API · webhook · OTA

`ESP32` · `ESP32-S2` · `ESP32-S3` · `ESP32-C3` · `ESP32-C5` · `ESP32-C6` — Arduino-ESP32 core 3.x

[English](README.md) · **Bahasa Indonesia**

</div>

---

## Daftar isi

1. [Mulai cepat](#mulai-cepat)
2. [Fitur](#fitur)
3. [Gambaran arsitektur](#gambaran-arsitektur)
4. [Instalasi](#instalasi)
5. [Contoh program](#contoh-program)
6. [Widget dashboard](#widget-dashboard)
7. [Lampu (LED)](#lampu-led)
8. [Alur visual (Flows)](#alur-visual-flows)
9. [Perangkat tambahan](#perangkat-tambahan)
10. [API key lisensi dan edisi](#api-key-lisensi-dan-edisi)
11. [Integrasi: MQTT, HTTP, webhook](#integrasi)
12. [Pemakaian modular](#pemakaian-modular)
13. [Peta dokumentasi](#peta-dokumentasi)
14. [Kendala umum](#kendala-umum)
15. [Lisensi](#lisensi)

---

## Mulai cepat

```cpp
#include <FlyghtNinno.h>

fn::Device dev("smart-lamp");

void setup() {
  pinMode(2, OUTPUT);
  dev.page("Kontrol").toggle("lampu", "Lampu Teras", [](bool on) { digitalWrite(2, on); });
  dev.license("fnk_live_...");  // API key kamu; tanpa key: evaluasi 30 menit
  dev.begin();
}

void loop() { dev.loop(); }
```

| Langkah | Yang Anda lakukan | Yang terjadi |
|:-:|---|---|
| 1 | Upload (Partition Scheme **"No FS 4MB (2MB APP x2)"**) | Board memancarkan Wi-Fi sendiri `smart-lamp-XXXX` |
| 2 | Sambungkan ponsel ke Wi-Fi itu | Dashboard **terbuka otomatis** (captive portal) |
| 3 | Buat password admin | Langsung masuk; sesi tersimpan 30 hari |
| 4 | Pilih Wi-Fi rumah | Buka `http://smart-lamp-xxxx.local` dari perangkat mana pun di jaringan itu |

Tersambung ulang atau board restart? Tidak perlu login lagi.

## Fitur

| Bidang | Isi |
|---|---|
| **Setup & jaringan** | Access point untuk setup, captive portal (Android, iOS/macOS, Windows, Firefox + DHCP option 114), simpan sampai 8 Wi-Fi dengan prioritas dan pindah otomatis, mDNS `nama.local` |
| **Login** | Password admin (PBKDF2), sesi bertanda tangan HMAC yang bertahan setelah restart, pembatasan percobaan login, akses tamu (opsional), akun & peran (opsional) |
| **Dashboard** | Toggle, tombol, slider, gauge, nilai, grafik, pilihan, teks — dideklarasikan di C++. Banyak halaman, update langsung lewat WebSocket, tata letak drag-and-drop disimpan di perangkat, tema terang/gelap, Inggris + Indonesia |
| **Lampu** | LED biasa (digital/PWM) dan LED RGB (piksel bawaan atau 3 pin), color picker, kecerahan, 12 efek non-blocking |
| **Alur visual** | Editor node di browser: pemicu → logika → aksi, cabang serial atau paralel, opsional thread FreeRTOS per alur |
| **Perangkat** | IMU BNO055, LCD I2C, kartu SD, layar TFT, GPS dengan peta, MCU eksternal lewat UART — masing-masing punya halaman |
| **Integrasi** | MQTT (topik state/set, retained, LWT, discovery Home Assistant), HTTP REST dengan token, webhook bertanda tangan |
| **Sistem** | Kesehatan modul, log langsung, update OTA, restart, reset pabrik, jam dari NTP atau browser |

## Gambaran arsitektur

```mermaid
flowchart LR
    subgraph Ponsel["Ponsel / browser"]
        UI["Web UI<br/>(disajikan dari flash)"]
    end
    subgraph Board["ESP32"]
        direction TB
        APP["fn::App / fn::Device<br/>wadah modul"]
        NET["Jaringan<br/>Wifi · Ap · Captive · Mdns"]
        WEB["Web<br/>HTTP + WebSocket"]
        AUTH["Auth<br/>sesi · peran"]
        DASH["Ui<br/>widget"]
        INT["Mqtt · Webhook"]
        DEV["Perangkat<br/>Light · Lcd · Tft · Gps ..."]
        FLOW["Flows<br/>otomasi visual"]
        NVS[("NVS<br/>pengaturan")]
        APP --> NET & WEB & AUTH & DASH & INT & DEV & FLOW
        APP -.-> NVS
    end
    UI <-->|"HTTP · WS"| WEB
    INT <-->|MQTT| BROKER[("Broker MQTT /<br/>Home Assistant")]
    INT -->|webhook| SERVER["Server Anda"]
    ANDA["Sketch Anda<br/>setup() / loop()"] --> APP
```

Setiap fitur adalah sebuah `fn::Module`: punya dependensi, dijalankan berurutan sesuai dependensi,
berjalan di `dev.loop()`, melaporkan kesehatan, dan menyimpan pengaturannya di NVS. Diagram lain:
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) (bahasa Inggris).

## Instalasi

| Alat | Caranya |
|---|---|
| Arduino IDE | *Sketch → Include Library → Add .ZIP Library…* lalu pilih `FlyghtNinno-<versi>.zip` (atau Library Manager bila sudah terdaftar) |
| arduino-cli | `arduino-cli lib install --zip-path FlyghtNinno-<versi>.zip` |
| PlatformIO | `lib_deps = flyght-ninno/FlyghtNinno` (framework Arduino, espressif32 dengan core Arduino 3.x) |

Kebutuhan: **Arduino-ESP32 core 3.3.x**. Paket rilis berisi header publik dan arsip precompiled
untuk tiap chip (`src/<chip>/libFlyghtNinno.a`).

**Partition scheme (penting).** Build lengkap berukuran 1,3–1,7 MB, lebih besar dari partisi
aplikasi bawaan 1,2 MB. Setiap contoh membawa `partitions.csv` (dua slot OTA 1,9 MB, tanpa SPIFFS),
dan batas upload juga perlu dinaikkan:

| Alat | Pengaturan |
|---|---|
| Arduino IDE | Tools → Partition Scheme → **No FS 4MB (2MB APP x2)** |
| arduino-cli | `--fqbn esp32:esp32:esp32s3:PartitionScheme=no_fs` |
| PlatformIO | `board_build.partitions = partitions.csv` |

## Contoh program

| Contoh | Isi | Perangkat keras |
|---|---|---|
| `01_SmartLamp` | Aplikasi terkecil: satu toggle | ESP32 + LED |
| `02_SensorMqtt` | Gauge, grafik, slider, pilihan, MQTT + HTTP | ESP32 apa saja |
| `03_CustomModule` | Membuat `fn::Module` sendiri | ESP32 apa saja |
| `04_ModularApp` | `fn::App` hanya dengan modul yang dipilih | ESP32 apa saja |
| `05_Devices` | IMU, LCD, SD, TFT, GPS, MCU dengan pin per chip | perangkat yang Anda punya |
| `06_BuiltinLed` | Lampu, color picker, efek, alur visual, tombol BOOT | **cukup board-nya** |

## Widget dashboard

```cpp
dev.page("Monitor", "activity")
   .gauge("suhu", "Suhu", "°C", 0, 50)
   .chart("riwayat", "Riwayat", "°C", 90).every(2000).size(2)
   .value("daya", "Daya", "W").decimals(0).icon("zap");

dev.page("Kontrol", "sliders")
   .toggle("pompa", "Pompa", [](bool on) { digitalWrite(POMPA, on); })
   .slider("kipas", "Kipas", 0, 100, 5, [](float v) { analogWrite(KIPAS, v * 2.55); }).unit("%")
   .select("mode", "Mode", {"Auto", "Manual", "Pergi"}, [](const String& m) { /* ... */ })
   .button("bel", "Bunyikan", [] { /* ... */ })
   .text("pesan", "Pesan LCD", [](const String& s) { /* ... */ }).role(fn::Role::Admin);

dev.ui().set("suhu", 24.5);   // dari loop(): dikirim langsung ke browser, MQTT dan webhook
```

| Widget | Nilai | Callback |
|---|---|---|
| `toggle(id, label, cb)` | nyala/mati | `void(bool)` |
| `button(id, label, cb)` | — | `void()` |
| `slider(id, label, min, max, step, cb)` | angka | `void(float)` |
| `select(id, label, {pilihan}, cb)` | teks | `void(const String&)` |
| `text(id, label, cb)` | teks | `void(const String&)` |
| `gauge(id, label, satuan, min, max)` | angka (hanya baca) | — |
| `value(id, label, satuan)` | angka atau teks (hanya baca) | — |
| `chart(id, label, satuan, titik)` | deret angka (hanya baca) | — |

Pengubah berlaku untuk widget terakhir: `size(1..3)`, `icon(nama)`, `unit()`, `decimals()`,
`role(peranMinimal)`, `every(ms)`. Daftar lengkap: [docs/REFERENCE.md](docs/REFERENCE.md#page--widgets).

## Lampu (LED)

```cpp
fn::RgbLed piksel(RGB_BUILTIN);       // WS2812 bawaan (DevKit S3 / C3 / C6)
fn::Led status(2);                    // LED biasa dengan PWM (pwm=false → digitalWrite)
fn::RgbLed strip(5, 6, 7, "meja");    // pin R, G, B (commonAnode=true bila perlu)
dev.use(piksel).use(status);

piksel.color(0x2563eb).brightness(60);
piksel.effect(fn::Fx::Breathe, 3000);
```

| Efek | Warna | Kecepatan bawaan | | Efek | Warna | Kecepatan bawaan |
|---|:-:|--:|---|---|:-:|--:|
| `solid` | 1 | — | | `cycle` (dua warna) | 2 | 2000 ms |
| `blink` | 1 | 1000 ms | | `candle` (lilin) | 1 | 1000 ms |
| `breathe` | 1 | 3000 ms | | `police` | 2 | 800 ms |
| `heartbeat` | 1 | 1200 ms | | `sos` | 1 | 200 ms / titik |
| `strobe` | 1 | 120 ms | | `sunrise` | otomatis | 60 s |
| `rainbow` | otomatis | 6000 ms | | `alarm` | 1 | 400 ms |

Efek dihitung dari `millis()` sehingga tidak pernah memblokir `loop()`, dan pratinjau di halaman
**Lights** sama persis dengan LED aslinya. Setiap lampu juga bisa dikontrol lewat HTTP
(`/fn/api/v1/lights/<id>`), MQTT (`<base>/light/<id>/set`) dan blok alur. Kondisi terakhir diingat.

## Alur visual (Flows)

```cpp
fn::Flows flows;
dev.license(FN_LICENSE_KEY);         // alur visual adalah fitur Pro (key Pro)...
dev.features().defaultOn("flows");   // ...dan defaultnya mati
dev.use(flows);
```

Buka **Devices → Flows**, mulai dari contoh, tarik blok dari palet, sambungkan titik kanan sebuah
blok ke titik kiri blok lain, lalu tekan **Run**.

```mermaid
flowchart LR
    T1(["Tiap 1000 ms"]) --> L1["Lampu: merah, toggle"]
    T2(["Tombol GPIO0"]) --> S{"Split<br/>serial"}
    S -->|1| A["Lampu: merah"] --> W1["Tunggu 1,5 s"]
    S -->|2| B["Lampu: kuning"] --> W2["Tunggu 0,6 s"]
    S -->|3| C["Lampu: hijau"]
    T3(["HTTP hook 'alert'"]) --> R{"Ulangi 3×"}
    R -->|each| D["Lampu: merah"] --> W3["Tunggu"] --> E["Lampu: mati"]
    R -->|done| M["Kirim MQTT"]
```

| Kelompok | Blok |
|---|---|
| **Pemicu** | Tombol Run · Saat start-up · Tiap N ms · Widget berubah · Pesan MQTT · HTTP hook · Tombol GPIO |
| **Logika** | Tunggu · If · Ulangi · Split (paralel / serial) · Set nilai |
| **Aksi** | Log · Set widget · Output GPIO · Kirim MQTT · Request HTTP |
| **Perangkat** | Lampu · Efek lampu · Teks LCD · Teks layar · Kirim ke MCU (dan blok dari modul Anda) |

* **Paralel**: beberapa kabel dari satu titik berjalan bersamaan. **Serial**: blok Split mode
  *serial* menjalankan output 1, lalu 2, lalu 3 (cabang beserta waktu tunggunya selesai dulu).
* **Thread**: alur berjalan di loop utama atau, di edisi Enterprise, di task FreeRTOS sendiri.
* **Aman**: blok tidak bisa memakai pin yang dipakai modul (LED, bus), flash SPI atau USB; timer
  minimal 50 ms; maksimal 8 alur × 40 blok, 16 jalannya alur bersamaan per alur.
* Kolom teks menerima `{value}` `{uptime}` `{time}` `{id}` `{name}` (dan `{base}` di topik MQTT).

## Perangkat tambahan

| Bus | Modul | Halaman | Yang didapat |
|---|---|---|---|
| I2C | `fn::Bno055 imu;` | Motion | Horizon, kompas, kotak 3D, kalibrasi, "jadikan datar" |
| I2C | `fn::LcdI2c lcd(16, 2);` | LCD | Cermin layar, tulis per baris, lampu latar, pemindai I2C |
| SPI / SDMMC | `fn::SdCard sd(cs);` | Files | Jelajah, unggah, unduh, ganti nama, hapus, pratinjau |
| SPI | `fn::Tft tft(panel);` | Screen | Cermin piksel dan alat gambar |
| UART | `fn::Gps gps(rx, tx);` | Map | Peta Leaflet, jejak, kecepatan, satelit, ekspor GPX |
| UART | `fn::McuLink mcu(rx, tx);` | Serial | Konsol, telemetri jadi nilai langsung, tombol cepat |
| GPIO | `fn::Led` / `fn::RgbLed` | Lights | Color picker, kecerahan, efek |

Perangkat hanya ikut ke firmware bila Anda membuat objeknya dan memanggil `dev.use(...)`. Pin:
`imu.pins(sda, scl)`, `sd.pins(sck, miso, mosi)`, `panel.pins(sck, mosi, miso)`; `examples/05_Devices`
berisi pin bawaan untuk tiap chip.

## API key lisensi dan edisi

Setiap build memerlukan **API key lisensi**: `fnk_live_` diikuti sekitar 150-180 karakter. Key
memuat nama pemilik dan edisi yang dibuka. Key ditandatangani flyght-ninno dengan **ECDSA P-256**
dan diperiksa **offline** di board saat boot, jadi tidak bisa diubah atau dipalsukan, dan tidak
perlu internet. Key Free gratis; key Pro dan Enterprise datang bersama lisensi komersial dan
berlaku untuk rilis yang terbit sampai tanggal update-nya.

```cpp
// secrets.h, di samping sketch dan tidak ikut version control:
#define FN_LICENSE_KEY "fnk_live_..."

dev.license(FN_LICENSE_KEY);             // sebelum dev.begin()
```

| Kondisi | Yang terjadi |
|---|---|
| Key valid | Edisinya dipakai; *Settings → Plan & features* menampilkan "Lisensi atas nama …" |
| Tanpa key, key salah ketik, dicabut atau palsu, atau rilis lebih baru dari tanggal update key | **Mode evaluasi**: semua fitur aktif, ada banner di dashboard, dan setelah **30 menit** aplikasi berhenti (HTTP 402) sampai reset berikutnya |

Log boot menyebutkan kondisi mana yang berlaku dan alasannya. Contoh-contoh membaca key dari
`secrets.h` bila file itu ada. Pada produk, aktifkan Flash Encryption agar key tidak bisa dibaca
dari dump flash.

### Saklar fitur


| Fitur | id | Edisi | Default |
|---|---|---|:-:|
| Akses tamu (tanpa password) | `guest` | Free | mati |
| Token API untuk sistem lain | `apiTokens` | Free | **nyala** |
| Alur visual | `flows` | Pro | mati |
| Akun & peran (viewer / operator / admin) | `rbac` | Pro | mati |
| White label (sembunyikan merek) | `whitelabel` | Pro | mati |
| Alur di thread RTOS | `flowsRtos` | Enterprise | mati |

```cpp
dev.features().defaultOn("rbac");        // default boot pertama; selanjutnya: Settings → Plan & features
if (dev.features().on("rbac")) { ... }   // diizinkan edisi DAN dinyalakan
```

## Integrasi

**MQTT** — topik dasar bawaan `flyght/{id}`.

| Topik | Arah | Isi |
|---|---|---|
| `<base>/status` | perangkat → | `online` / `offline` (retained, LWT) |
| `<base>/state/<widget>` | perangkat → | `ON`/`OFF`, angka atau teks |
| `<base>/set/<widget>` | → perangkat | sama, plus `toggle` dan `PRESS` |
| `<base>/light/<id>/set` | → perangkat | `ON`, `OFF`, `TOGGLE`, `#rrggbb`, nama efek atau JSON |
| `<base>/light/<id>/state` | perangkat → | JSON kondisi lampu (retained) |

**HTTP** — semua endpoint ada di [docs/API.md](docs/API.md).

```bash
curl -H "Authorization: Bearer $TOKEN" http://smart-lamp-c2d3.local/fn/api/v1/state
curl -X POST -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
     -d '{"lampu": true, "kipas": 40}' http://smart-lamp-c2d3.local/fn/api/v1/state
curl -X POST -H "Authorization: Bearer $TOKEN" http://smart-lamp-c2d3.local/fn/api/v1/flows/hook/alert
```

**Webhook** — POST JSON saat ada perubahan atau berkala, ditandatangani HMAC-SHA256 (`X-Flyght-Signature`).

## Pemakaian modular

```cpp
fn::App app("field-meter");
fn::Web web;  fn::Auth auth;  fn::System sys;  fn::Ap ap;  fn::Captive captive;  fn::Mdns mdns;  fn::Ui ui;
app.use(web).use(auth).use(sys).use(ap).use(captive).use(mdns).use(ui);
app.begin();
```

Modul buatan sendiri cukup punya `id()` dan `begin()`; `loop()`, `deps()`, `health()` dan blok alur
bersifat opsional. Lihat [docs/MODULES.md](docs/MODULES.md) dan `examples/03_CustomModule`.

## Peta dokumentasi

| Dokumen | Untuk |
|---|---|
| [docs/REFERENCE.md](docs/REFERENCE.md) | Semua kelas dan fungsi yang dipanggil dari sketch, lengkap dengan tabel |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Diagram blok: modul, boot, request, sesi, alur, thread |
| [docs/API.md](docs/API.md) | Endpoint HTTP, topik WebSocket, isi webhook |
| [docs/MODULES.md](docs/MODULES.md) | Membuat modul, halaman UI dan blok alur |
| [CHANGELOG.md](CHANGELOG.md) | Riwayat perubahan |

Dokumen di folder `docs/` berbahasa Inggris; nama fungsi dan contoh kodenya sama untuk semua bahasa.

## Kendala umum

| Gejala | Solusi |
|---|---|
| "Sketch too big" | Partition Scheme → *No FS 4MB (2MB APP x2)* |
| Serial Monitor kosong pada board USB native | Tools → *USB CDC On Boot: Enabled* |
| Halaman tidak terbuka sendiri di ponsel | Buka `http://192.168.4.1` manual; beberapa ponsel perlu data seluler dimatikan |
| `nama.local` tidak bisa dibuka | Pakai IP yang tampil di halaman Wi-Fi (Android < 12 belum mendukung mDNS) |
| Alur tidak bisa disimpan: "GPIO N is used by …" | Pin itu dipakai LED, bus, flash atau USB; pilih pin lain |
| Merah dan hijau tertukar pada LED RGB | `led.colorOrder(fn::ColorOrder::RGB)` atau halaman Lights → Colour order → RGB |
| `#error ... built for the ESP32 Arduino core 3.3.x` | Pasang versi core tersebut di Boards Manager |

## Lisensi

**FlyghtNinno Community License 1.1** — gratis dipakai untuk membuat dan menjual firmware, termasuk
produk komersial, dengan API key lisensi; library-nya sendiri tidak boleh disebarkan ulang, dimodifikasi untuk disebarkan,
di-reverse engineer, dipakai membuat library pesaing, atau dijalankan dengan pemeriksaan lisensi yang dilewati. Fitur Pro/Enterprise pada produk yang
dijual memerlukan lisensi komersial. Lihat [LICENSE](LICENSE) dan [NOTICE](NOTICE).

Aset pihak ketiga: Montserrat (SIL OFL 1.1), Leaflet 1.9.4 (BSD-2-Clause, hanya dengan `fn::Gps`).
Peta © kontributor OpenStreetMap.

© 2026 flyght-ninno
