// FlyghtEdge — pins of a device module that an admin may move from the dashboard.
//
// A module that drives or reads GPIO pins (Relay, and later the sensors and motors) implements
// PinDevice and registers itself with pins::add() in begin(). The card menu (the three dots) then
// opens a Settings dialog that lists its pins and offers only the pins that can work for each role.
//
//   GET /fn/api/v1/devices                      admin: devices, their pins, and the pins each role may use
//   PUT /fn/api/v1/devices/<id>/pins            admin: {"pins":{"ch1":4,"ch2":null},"confirm":"<device name>"}
//
// A saved assignment wins over the pins the sketch passes, like every other setting. GPIO pins apply at
// once on the loop task; a module that needs a restart for its pins (a bus) says so in the answer.
#pragma once

#include <vector>

#include "../core/App.h"
#include "../pure/PureTypes.h"

namespace fn {

struct PinSlot {
  String key;     // "ch1", "trig": stored and sent over HTTP; letters, digits and underscore
  String label;   // shown in the dialog
  pure::PinRole role = pure::PinRole::Out;
  String proto = "gpio";   // what the pin is for, shown in the dialog: gpio, uart-rx, uart-tx, i2c-sda, i2c-scl, onewire, step, dir ...
  int pin = -1;   // in use now (-1 = none)
  int def = -1;   // what the sketch asked for
};

class PinDevice {
public:
  virtual ~PinDevice() = default;
  // At most 15 characters: it is the storage key. Also the owner name of the pins in the registry.
  virtual const char* pinDeviceId() const = 0;
  virtual const char* pinDeviceTitle() const = 0;
  // true when a wrong pin can switch a load: a change then needs the device name as confirmation
  virtual bool pinsDriveLoad() const { return false; }
  // true for a bus (UART, I2C): the pins are saved now and used from the next start
  virtual bool pinsNeedRestart() const { return false; }
  virtual void pinSlots(std::vector<PinSlot>& out) const = 0;
  // Loop task. The pins are in pinSlots() order and were checked. Put the old pins in a safe state
  // before they are released.
  virtual void pinApply(const std::vector<int>& pins) = 0;
};

namespace pins {
// Registers the device and, once, the routes. Call from begin().
void add(App& app, PinDevice* device);
// The saved pin for (device, key), or `def` when nothing is saved. A saved -1 means "none".
int saved(const char* deviceId, const char* key, int def);
}  // namespace pins

}  // namespace fn
