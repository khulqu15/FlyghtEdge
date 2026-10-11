// FlyghtEdge — 05 Devices: IMU + LCD (I2C), SD + TFT (SPI), GPS + MCU (UART)
//
// Every device is optional: delete the lines you don't need and nothing of it
// ends up in the firmware. Each one adds its own page under "Devices":
//   Motion (BNO055) · LCD · Files (SD) · Screen (TFT) · Map (GPS) · Serial (MCU)
//
// Default pins below are picked per chip; change them to match your wiring.
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" (see partitions.csv).

#include <FlyghtEdge.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

// ---------------------------------------------------------------- pins ---
#if CONFIG_IDF_TARGET_ESP32
const int I2C_SDA = 21, I2C_SCL = 22;
const int SPI_SCK = 18, SPI_MISO = 19, SPI_MOSI = 23, SD_CS = 5;
const int TFT_CS = 15, TFT_DC = 2, TFT_RST = 4, TFT_BL = 32;
const int GPS_RX = 16, GPS_TX = 17, MCU_RX = 26, MCU_TX = 27;
#elif CONFIG_IDF_TARGET_ESP32S3
const int I2C_SDA = 8, I2C_SCL = 9;
const int SPI_SCK = 12, SPI_MISO = 13, SPI_MOSI = 11, SD_CS = 10;
const int TFT_CS = 14, TFT_DC = 15, TFT_RST = 16, TFT_BL = 21;
const int GPS_RX = 18, GPS_TX = 17, MCU_RX = 4, MCU_TX = 5;
#elif CONFIG_IDF_TARGET_ESP32S2
const int I2C_SDA = 8, I2C_SCL = 9;
const int SPI_SCK = 36, SPI_MISO = 37, SPI_MOSI = 35, SD_CS = 34;
const int TFT_CS = 33, TFT_DC = 21, TFT_RST = 16, TFT_BL = 15;
const int GPS_RX = 18, GPS_TX = 17, MCU_RX = 4, MCU_TX = 5;
#elif CONFIG_IDF_TARGET_ESP32C3
const int I2C_SDA = 8, I2C_SCL = 9;
const int SPI_SCK = 4, SPI_MISO = 5, SPI_MOSI = 6, SD_CS = 7;
const int TFT_CS = 1, TFT_DC = 2, TFT_RST = -1, TFT_BL = 3;
const int GPS_RX = 10, GPS_TX = 0, MCU_RX = 20, MCU_TX = 21;
#elif CONFIG_IDF_TARGET_ESP32C6
const int I2C_SDA = 23, I2C_SCL = 22;
const int SPI_SCK = 21, SPI_MISO = 20, SPI_MOSI = 19, SD_CS = 18;
const int TFT_CS = 1, TFT_DC = 2, TFT_RST = -1, TFT_BL = 3;
const int GPS_RX = 4, GPS_TX = 5, MCU_RX = 17, MCU_TX = 16;
#elif CONFIG_IDF_TARGET_ESP32C5
const int I2C_SDA = 0, I2C_SCL = 1;
const int SPI_SCK = 10, SPI_MISO = 9, SPI_MOSI = 8, SD_CS = 6;
const int TFT_CS = 7, TFT_DC = 3, TFT_RST = -1, TFT_BL = 2;
const int GPS_RX = 4, GPS_TX = 5, MCU_RX = 12, MCU_TX = 11;
#else  // other chips: board defaults
const int I2C_SDA = SDA, I2C_SCL = SCL;
const int SPI_SCK = SCK, SPI_MISO = MISO, SPI_MOSI = MOSI, SD_CS = SS;
const int TFT_CS = -1, TFT_DC = -1, TFT_RST = -1, TFT_BL = -1;
const int GPS_RX = RX, GPS_TX = TX, MCU_RX = -1, MCU_TX = -1;
#endif

// ESP32 / S3 have a third UART; on 2-UART chips the MCU link uses UART0,
// which is free when "USB CDC On Boot" is enabled (Serial = USB).
#if SOC_UART_HP_NUM > 2
#define MCU_PORT Serial2
#elif ARDUINO_USB_CDC_ON_BOOT
#define MCU_PORT Serial0
#endif

fn::Device dev("maker-lab");

fn::Bno055 imu;                    // I2C 0x28
fn::LcdI2c lcd(16, 2);             // I2C 0x27 / 0x3F (auto)
fn::SdCard sd(SD_CS);              // SPI
fn::TftSpi panel(fn::TftSpi::ST7789, 240, 240, TFT_CS, TFT_DC, TFT_RST);
fn::Tft tft(panel);                // SPI, shares the bus with the SD card
fn::Gps gps(GPS_RX, GPS_TX, 9600, Serial1);
#ifdef MCU_PORT
fn::McuLink mcu(MCU_RX, MCU_TX, 115200, MCU_PORT);
#endif

void setup() {
  Serial.begin(115200);

  imu.pins(I2C_SDA, I2C_SCL);
  lcd.pins(I2C_SDA, I2C_SCL);
  sd.pins(SPI_SCK, SPI_MISO, SPI_MOSI);
  panel.pins(SPI_SCK, SPI_MOSI, SPI_MISO);
  tft.backlight(TFT_BL);

  dev.page("Overview", "home")
      .gauge("pitch", "Pitch", "°", -90, 90).icon("compass")
      .gauge("roll", "Roll", "°", -90, 90).icon("compass")
      .value("speed", "GPS speed", "km/h").icon("map")
      .value("temp", "MCU temperature", "°C").icon("thermo")
      .toggle("light", "MCU light").icon("bulb");

  dev.use(imu).use(lcd).use(sd).use(tft).use(gps);
#ifdef MCU_PORT
  mcu.bind("temp", "temp").forward("light").quick("Ping", "PING").quick("Blink", "LED=BLINK");
  dev.use(mcu);
#endif
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last >= 500) {
    last = millis();
    if (imu.ready()) {
      dev.ui().set("pitch", imu.pitch());
      dev.ui().set("roll", imu.roll());
    }
    if (gps.hasFix()) dev.ui().set("speed", gps.fix().speedKmh);
    lcd.line(1, String("Up ") + millis() / 1000 + "s");
  }

  // Log the position once a minute while a card is inserted.
  static uint32_t lastLog = 0;
  if (sd.mounted() && gps.hasFix() && millis() - lastLog >= 60000) {
    lastLog = millis();
    fn::SdCard::Lock lock(sd);
    File f = sd.fs().open("/track.csv", FILE_APPEND);
    if (f) { f.printf("%lu,%.6f,%.6f\n", (unsigned long)fn::unixTime(), gps.lat(), gps.lon()); f.close(); }
  }
}
