// FlyghtEdge — 09 Motors: a DC motor, a brushless motor on an ESC, a stepper and a solenoid valve.
//
// Each motor has a card with its speed, position or throttle and a Stop button. An admin opens the
// three dots on a card > Settings to move a pin; the dialog asks for the device name because the pins
// switch a load, and the motor is stopped before a pin moves. Delete what you do not have.
//
//   DcMotor   H-bridge (L298N, TB6612, L9110 ...): speed -100..100 %, a reversal goes through zero
//   Esc       brushless motor through an ESC: arms first (idle pulse), then throttle 0..100 %
//   Stepper   A4988 / DRV8825 / TB6600: move to a position with acceleration (a few thousand steps/s)
//   Solenoid  valve that closes by itself and rests between openings
//
// TAKE THE PROPELLER OFF while you test an ESC. This is a convenience, not a safety function: an
// emergency stop belongs in hardware.
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
const int M_PWM = 25, M_IN1 = 26, M_IN2 = 27, ESC_PIN = 32, STEP = 14, DIR = 12, EN = 13, VALVE = 33;
#else
const int M_PWM = 4, M_IN1 = 5, M_IN2 = 6, ESC_PIN = 7, STEP = 15, DIR = 16, EN = 17, VALVE = 18;
#endif

fn::Device dev("motor-lab");

fn::DcMotor motor(M_PWM, M_IN1, M_IN2);
fn::Esc esc(ESC_PIN);
fn::Stepper stepper(STEP, DIR, EN);
fn::Solenoid valve(VALVE, "valve", "Water valve", 30, 5);   // closes after 30 s, rests 5 s

void setup() {
  Serial.begin(115200);

  motor.ramp(50).maxRun(600);                 // 2 s to full speed, stops by itself after 10 minutes
  esc.ramp(30).maxRun(300);
  stepper.maxSpeed(1000).acceleration(2000).limits(-4000, 4000);

  dev.license(FN_LICENSE_KEY);
  dev.use(motor).use(esc).use(stepper).use(valve);
  dev.begin();
}

void loop() { dev.loop(); }
