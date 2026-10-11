// FlyghtEdge — shared I2C helpers for device modules (BNO055, LCD, ...).
#pragma once

#include <Wire.h>

#include "../web/Web.h"

namespace fn {
namespace i2c {

// Starts a bus once, on custom pins or the board defaults (SDA/SCL of the
// selected board). Later calls for the same bus are no-ops.
bool begin(TwoWire& wire, int sda = -1, int scl = -1, uint32_t freq = 400000);
bool probe(TwoWire& wire, uint8_t addr);
// Best guess for well known addresses ("BNO055 IMU", "LCD backpack" ...).
const char* guess(uint8_t addr);
// Registers GET /fn/api/v1/i2c/scan once per firmware.
void routes(Web& web, TwoWire& wire);

bool writeReg(TwoWire& wire, uint8_t addr, uint8_t reg, uint8_t value);
bool readRegs(TwoWire& wire, uint8_t addr, uint8_t reg, uint8_t* out, size_t len);

}  // namespace i2c
}  // namespace fn
