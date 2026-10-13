// FlyghtEdge — 15 IMU tilt: pitch, roll and a tilt alarm from a BNO055.
//
// The BNO055 does its own sensor fusion, so pitch and roll are ready to use. The dashboard shows both as
// gauges and raises "TILTED" when either goes past TILT_LIMIT degrees. "Set level" makes the current
// attitude zero (mount the sensor, level the machine, press the button). Devices > Motion has the 3D
// box, the compass and the calibration status.
//
// Wiring (I2C):   SDA, SCL, VCC 3.3 V, GND. The default address is 0x28 (0x29 when ADR is high).
// Give the sensor a minute and a few slow turns to calibrate before you trust the heading.
//
// Status: compiled for ESP32-S3; not yet run on a board.
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" or use the partitions.csv here.

#include <FlyghtEdge.h>
#include <math.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

// ---- pins (the wiring diagram in docs/guides is generated from this block) ----
#if CONFIG_IDF_TARGET_ESP32
const int I2C_SDA = 21, I2C_SCL = 22;
#elif CONFIG_IDF_TARGET_ESP32S3
const int I2C_SDA = 8, I2C_SCL = 9;
#else  // ESP32-C3, C6, S2 ...
const int I2C_SDA = 8, I2C_SCL = 9;
#endif

const float TILT_LIMIT = 30.0f;   // degrees

fn::Device dev("imu-tilt");
fn::Bno055 imu;   // I2C, 0x28

void setup() {
  Serial.begin(115200);

  imu.pins(I2C_SDA, I2C_SCL);

  dev.page("Tilt", "compass")
      .gauge("pitch", "Pitch", "°", -90, 90).icon("compass")
      .gauge("roll", "Roll", "°", -90, 90).icon("compass")
      .value("alarm", "Tilt alarm").icon("alert")
      .button("level", "Set level", [] { imu.level(); }).icon("sliders");

  dev.use(imu);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 200) return;
  last = millis();

  if (!imu.ready()) {
    dev.ui().set("alarm", "Sensor not ready");
    return;
  }
  dev.ui().set("pitch", imu.pitch());
  dev.ui().set("roll", imu.roll());
  const bool tilted = fabsf(imu.pitch()) > TILT_LIMIT || fabsf(imu.roll()) > TILT_LIMIT;
  dev.ui().set("alarm", tilted ? "TILTED" : "OK");
}
