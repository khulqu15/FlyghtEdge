# FlyghtEdge guides

Plain-language guides for building with FlyghtEdge, in the order most people need them. Every guide has
wiring, the code that matters (taken from a real example that compiles) and what you will see on the
dashboard.

| # | Guide | Read it when |
|---|---|---|
| 1 | [Getting started](01-getting-started.md) | you have a board and want a dashboard on your phone |
| 2 | [Dashboard and pages](02-dashboard-and-pages.md) | you want to know what widgets there are and how they behave |
| 3 | [Wi-Fi and provisioning](03-wifi-and-provisioning.md) | you need several networks, or to change the setup network |
| 4 | [GPS](04-gps.md) | you want a map, a track or a geofence |
| 5 | [TFT screen](05-tft.md) | you want a small colour display |
| 6 | [SD card](06-sd-card.md) | you want to log data or browse files |
| 7 | [LCD and IMU](07-lcd-and-imu.md) | you want a text display or a tilt sensor |
| 8 | [MCU link](08-mcu-link.md) | another microcontroller holds some of your parts |
| 9 | [MQTT and Home Assistant](09-mqtt-and-home-assistant.md) | you use a broker or Home Assistant |
| 10 | [Webhooks and flows](10-webhooks-and-flows.md) | you want to notify another system or automate on the device |
| 11 | [Relays, sensors and motors](11-relays-sensors-motors.md) | you are building a real project |
| 12 | [Flyght Cloud](12-flyght-cloud.md) | you want to reach the device from anywhere |
| 13 | [Wiring and pinout](13-wiring-and-pinout.md) | you are about to connect wires |
| 14 | [Troubleshooting](14-troubleshooting.md) | something does not work |
| 15 | [Screen catalog](15-screen-catalog.md) | you want to see every screen, on a desktop and on a phone |

Also: the generated [pin tables](pin-tables.md), the [C++ reference](../REFERENCE.md), the
[HTTP API](../API.md) and the [architecture notes](../ARCHITECTURE.md).

## About the pictures

- **Screenshots** are taken from the simulator that ships with the project, which serves the same
  dashboard as the firmware. Values are sample data. The guides show the desktop view; the
  [screen catalog](15-screen-catalog.md) has the desktop and the phone view of every screen.
- **Wiring diagrams and GPIO maps** are drawn from the sketches' own pin blocks and from the pin rules
  of the firmware. They are by GPIO number: match your board by GPIO number, not by pin position.
- **Examples** are in `examples/`. Each one says in its header whether it has run on hardware.

<!-- flyght-cloud-footer:start -->
---

**Flyght Cloud.** See this device next to your others, from anywhere: [flyght.obayan.id](https://flyght.obayan.id). Sign in, claim the device with its link, and the dashboard you built here is also on the web.
<!-- flyght-cloud-footer:end -->
