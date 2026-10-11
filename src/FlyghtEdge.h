// FlyghtEdge — ESP32 dashboard on your phone in 5 lines.
// https://github.com/flyght-ninno/FlyghtNinno
// (c) 2026 flyght-ninno. FlyghtEdge Community License 1.1 - see LICENSE.
//
// Quick start:
//
//   #include <FlyghtEdge.h>
//   fn::Device dev("smart-lamp");
//   void setup() {
//     pinMode(2, OUTPUT);
//     dev.page("Kontrol").toggle("light", "Light", [](bool on) { digitalWrite(2, on); });
//     dev.license("fnk_live_...");   // license key; without one: 30-minute evaluation
//     dev.begin();
//   }
//   void loop() { dev.loop(); }
//
// Join the "smart-lamp-XXXX" Wi-Fi from your phone: the dashboard opens by
// itself. Modules live under fn/ and can be used one by one with fn::App.
#pragma once

#include <esp_arduino_version.h>
#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
#error "FlyghtEdge needs the ESP32 Arduino core 3.x (Tools > Board > Boards Manager > esp32)"
#endif

#include "fn/core/Deps.h"  // core libraries the (precompiled) library links against
#include "fn/Device.h"

// Optional devices: each is linked into the firmware only when you create it
// and add it with dev.use(...). Their web pages appear under "Devices".
#include "fn/dev/Bno055.h"   // I2C  IMU: horizon, compass, 3D box
#include "fn/dev/LcdI2c.h"   // I2C  HD44780 LCD mirror
#include "fn/dev/SdCard.h"   // SPI / SDMMC file manager
#include "fn/dev/Tft.h"      // SPI  ST7789 / ILI9341 / any GFX: mirror + drawing
#include "fn/dev/Gps.h"      // UART GPS map (Leaflet)
#include "fn/dev/McuLink.h"  // UART external MCU console + telemetry
#include "fn/dev/Light.h"    // LEDs: plain (digital / PWM) and RGB, colour picker + effects
#include "fn/dev/Sensors.h"  // DHT, DS18B20, ultrasonic, analog (water level, gas, current), digital, MH-Z19 CO2, PZEM, INA219
#include "fn/dev/Motors.h"   // DC motor (H-bridge), brushless (ESC), stepper (step/dir); pins movable from the card menu
#include "fn/dev/Relay.h"    // 1-16 relay channels (active-high / low, latching), pins movable from the card menu

// Visual flows (Pro): connect triggers, logic and device blocks in the browser.
#include "fn/flow/Flows.h"
