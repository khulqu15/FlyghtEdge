// FlyghtEdge — 08 Sensors: temperature, humidity, distance, tank level, gas, power ... on one dashboard.
//
// Every sensor gets a card per reading ("—" while there is no trustworthy value). An admin opens the
// three dots on a card > Settings to move a pin without recompiling; UART and I2C pins are used from
// the next start. Delete the lines of the sensors you do not have: nothing of them is linked.
//
//   Dht           DHT22 temperature + humidity              1 pin
//   Ds18b20       DS18B20 temperature (4.7 k pull-up)       1 pin
//   Ultrasonic    HC-SR04 / JSN-SR04T distance + tank level trig + echo (use a divider on 5 V echo!)
//   AnalogSensor  water level (4-20 mA / 0.5-4.5 V), MQ gas, O2, ACS712 current; ADC1 pins only
//   DigitalSensor float switch, water leak
//   Mhz19         CO2                                        UART
//   Pzem          mains voltage / current / power / energy   UART
//   Ina219        DC voltage / current / power               I2C
//
// Calibration is yours: a gas sensor shows what its curve says. Check it against a reference.
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" or use the partitions.csv here.

#include <FlyghtEdge.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

#if CONFIG_IDF_TARGET_ESP32
const int DHT_PIN = 4, OW_PIN = 5, TRIG = 18, ECHO = 19, TANK = 34, LEAK = 23, CO2_RX = 16, CO2_TX = 17;
#else
const int DHT_PIN = 4, OW_PIN = 5, TRIG = 6, ECHO = 7, TANK = 1, LEAK = 15, CO2_RX = 16, CO2_TX = 17;
#endif

fn::Device dev("sensor-lab");

fn::Dht dht(DHT_PIN, fn::Dht::Type::Dht22);
fn::Ds18b20 probe(OW_PIN);
fn::Ultrasonic tankDistance(TRIG, ECHO, "tank");
fn::AnalogSensor pressure(TANK, "pressure", "Tank pressure", "m");
fn::DigitalSensor leak(LEAK, "leak", "Water leak");
fn::Mhz19 co2(CO2_RX, CO2_TX);

void setup() {
  Serial.begin(115200);

  dht.every(5000).alarm(35, 32);                  // an alarm card, on at 35 C, off again at 32 C
  probe.every(5000);
  tankDistance.tank(150, 20);                     // sensor on top: 150 cm = empty, 20 cm = full
  pressure.map(500, 4500, 0, 3).range(0, 3, 2);   // 0.5-4.5 V = 0-3 m of water (after a divider to 3.3 V)
  leak.activeLow().labels("WET", "dry").onChange([](bool wet) { if (wet) Serial.println("water!"); });

  dev.license(FN_LICENSE_KEY);
  dev.use(dht).use(probe).use(tankDistance).use(pressure).use(leak).use(co2);
  dev.begin();
}

void loop() { dev.loop(); }
