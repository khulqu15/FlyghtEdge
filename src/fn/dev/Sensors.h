// FlyghtEdge — sensors. Each class is its own module and its own object file: only the ones the
// sketch creates end up in the firmware.
//
//   Dht           temperature + humidity           DHT11 / DHT22, one pin
//   Ds18b20       temperature                      one DS18B20 on a 1-Wire pin (4.7 k pull-up)
//   Ultrasonic    distance (and tank level)        HC-SR04, JSN-SR04T, any trig + echo module
//   AnalogSensor  anything with an analog output   water level, O2, H2, MQ gas, ACS712 current ...
//   DigitalSensor on / off input                   float switch, water leak, flame, door
//   Mhz19         CO2                              UART, 9600 baud
//   Pzem          mains voltage, current, power    PZEM-004T v3, UART, 9600 baud
//   Ina219        DC voltage, current, power       I2C, 0x40
//
// Every sensor has a card per reading, an alarm if you ask for one, and its pins can be moved by an
// admin from the card menu (the three dots > Settings); a saved pin wins over the one in the sketch.
// UART and I2C pins are saved at once and used from the next start.
//
// Calibration is yours: a gas sensor reads what its curve says, not what a lab says. The library
// shows "—" instead of a number whenever a reading cannot be trusted.
#pragma once

#include <HardwareSerial.h>
#include <Wire.h>

#include <functional>

#include "Sensor.h"

namespace fn {

class Dht : public Sensor {
public:
  enum class Type : uint8_t { Dht11, Dht22 };
  explicit Dht(int pin, Type type = Type::Dht22, const char* id = "dht");
  float temperature() const { return value(0); }
  float humidity() const { return value(1); }

protected:
  bool hw() override;
  void unhw() override;
  void tick(uint32_t now) override;

private:
  bool readFrame(uint8_t f[5]);
  Type type_;
  bool starting_ = false;
  uint32_t at_ = 0;
};

class Ds18b20 : public Sensor {
public:
  explicit Ds18b20(int pin, const char* id = "ds18b20");
  float temperature() const { return value(0); }

protected:
  bool hw() override;
  void unhw() override;
  void tick(uint32_t now) override;

private:
  bool converting_ = false;
  uint32_t at_ = 0;
};

class Ultrasonic : public Sensor {
public:
  Ultrasonic(int trigPin, int echoPin, const char* id = "distance");
  // One ultrasonic sensor per firmware (the echo interrupt keeps its pin in one place).
  // Speed of sound depends on the air: give the temperature if you know it (default 20 C).
  Ultrasonic& airTemperature(float celsius) { airC_ = celsius; return *this; }
  // A tank with the sensor on top: adds a "level" in percent between `emptyCm` and `fullCm`.
  Ultrasonic& tank(float emptyCm, float fullCm);
  float distanceCm() const { return value(0); }

protected:
  bool hw() override;
  void unhw() override;
  void tick(uint32_t now) override;

private:
  static void isr(void* self);
  volatile uint32_t riseUs_ = 0, widthUs_ = 0;
  volatile bool done_ = false;
  bool waiting_ = false;
  uint32_t at_ = 0;
  float airC_ = NAN, emptyCm_ = NAN, fullCm_ = NAN;
  int level_ = -1;
};

class AnalogSensor : public Sensor {
public:
  // `pin` must be an ADC1 pin (the library refuses the others: ADC2 stops working with Wi-Fi).
  AnalogSensor(int pin, const char* id, const char* title, const char* unit);
  // Linear: the sensor gives `mvLo`..`mvHi` millivolts for `outLo`..`outHi` (the value is clamped).
  AnalogSensor& map(float mvLo, float mvHi, float outLo, float outHi);
  // Gauge scale and decimals of the card.
  AnalogSensor& range(float lo, float hi, uint8_t decimals);
  // Hall current sensor (ACS712 ...): amps = (mV - zeroMv) / mvPerAmp. Mind the divider if the board is 5 V.
  AnalogSensor& hall(float zeroMv, float mvPerAmp);
  // Metal-oxide gas sensor on a divider: ppm = a * (Rs/R0)^b with Rs = RL * (Vc - Vout) / Vout.
  AnalogSensor& mq(float supplyV, float loadOhm, float r0Ohm, float a, float b);
  float millivolts() const { return mv_; }

protected:
  bool hw() override;
  void tick(uint32_t now) override;

private:
  enum class Mode : uint8_t { Linear, Hall, Mq };
  Mode mode_ = Mode::Linear;
  float mvLo_ = 0, mvHi_ = 3300, outLo_ = 0, outHi_ = 100;
  float zeroMv_ = 0, mvPerAmp_ = 100;
  float vc_ = 5, rl_ = 10000, r0_ = 10000, a_ = 100, b_ = -1.5f;
  float mv_ = NAN;
  const char* unit_;
};

class DigitalSensor : public Sensor {
public:
  DigitalSensor(int pin, const char* id, const char* title);
  DigitalSensor& activeLow(bool on = true) { activeLow_ = on; return *this; }   // wired to ground when active
  DigitalSensor& pullUp(bool on = true) { pullUp_ = on; return *this; }
  DigitalSensor& labels(const char* active, const char* idle) { on_ = active; off_ = idle; return *this; }
  DigitalSensor& debounce(uint32_t ms) { holdMs_ = ms; return *this; }
  DigitalSensor& onChange(std::function<void(bool)> cb) { cb_ = std::move(cb); return *this; }
  bool active() const { return deb_.stable; }

protected:
  bool hw() override;
  void tick(uint32_t now) override;

private:
  bool activeLow_ = false, pullUp_ = true, first_ = true;
  const char* on_ = "Active";
  const char* off_ = "Idle";
  uint32_t holdMs_ = 50;
  pure::Debounce deb_;
  std::function<void(bool)> cb_;
};

class Mhz19 : public Sensor {
public:
  Mhz19(int rxPin, int txPin, HardwareSerial& port = Serial2, const char* id = "co2");
  int ppm() const { return std::isnan(value(0)) ? -1 : (int)value(0); }

protected:
  bool hw() override;
  void unhw() override;
  void tick(uint32_t now) override;

private:
  HardwareSerial& port_;
  uint8_t buf_[9];
  uint8_t n_ = 0;
  bool asked_ = false;
  uint32_t at_ = 0;
};

class Pzem : public Sensor {
public:
  Pzem(int rxPin, int txPin, HardwareSerial& port = Serial2, uint8_t address = 0xF8, const char* id = "mains");
  float volts() const { return value(0); }
  float amps() const { return value(1); }
  float watts() const { return value(2); }

protected:
  bool hw() override;
  void unhw() override;
  void tick(uint32_t now) override;

private:
  HardwareSerial& port_;
  uint8_t addr_;
  uint8_t buf_[25];
  uint8_t n_ = 0;
  bool asked_ = false;
  uint32_t at_ = 0;
};

class Ina219 : public Sensor {
public:
  // `shuntOhm` is the resistor on the board (0.1 ohm on most breakouts, 0.01 on the 8 A ones).
  Ina219(uint8_t address = 0x40, float shuntOhm = 0.1f, TwoWire& wire = Wire, int sda = -1, int scl = -1, const char* id = "ina219");
  float volts() const { return value(0); }
  float amps() const { return value(1); }
  float watts() const { return value(2); }

protected:
  bool hw() override;
  void tick(uint32_t now) override;

private:
  TwoWire& wire_;
  uint8_t addr_;
  float shuntOhm_;
};

}  // namespace fn
