# Changelog

## 2.3.0 — 2026-10-16

- **Servos.** `fn::Servo` drives a hobby servo on a 50 Hz signal, positional or 360 continuous, with
  presets for MG90S, MG995, MG996R and SG90 and a calibration dialog that saves the pulse window per
  servo; the pin is moved from the card like every other device. The `09_Motors` example and the
  simulator show it.
- **The cloud sees how a device is wired.** Once per connect, after the claim, the device publishes its
  hardware inventory on `<base>/hw` (retained): the chip, every device with its pins, roles and
  protocols, and the pins still free for each role — the same inventory `GET /fn/api/v1/devices` returns.
- **The Flyght Cloud address is server-managed.** The dashboard card shows it but no longer offers it
  as a setting, and `PUT /fn/api/v1/cloud` answers 403 to a body carrying `site` without changing
  anything else in that request. A device still follows the address the server names in its claim and
  config answers, and still falls back to the built-in one when that address stops answering. Point a
  build of your own somewhere else with `dev.cloud().site(...)` or `-DFN_CLOUD_SITE=...`. Scripts that
  set the address through the local API need to change; a device that saved an address from the old
  dashboard keeps it until the server names another one or it is reset.

## 2.2.0 — 2026-10-13

- **One menu on every screen size.** On a phone the side menu is a drawer you open with the menu button (rooms, device pages, Wi-Fi, Connect, Settings). The bottom tab bar and the page tabs on the dashboard are gone.
- **Settings is grouped**: Device & health, Cloud & license, Accounts & security, Maintenance & factory reset, with a row that jumps to a group and groups that fold. *Users & roles* is always there for the admin, with a switch to turn it on (or a note when the edition does not include it).
- **Setup on the evaluation banner**: next to *Enter license key*, it erases the settings after you type RESET and starts the setup wizard again.
- **Calibrate colours** on the Lights page: the LED shows its colours one at a time, you pick the one you see, and the colour order is saved after a final "is this red?" check.

## 2.1.1 — 2026-10-13

- **Enter the license key from the dashboard.** In evaluation mode the banner has an *Enter license key* button, and a device whose evaluation ran out shows the same button, so it no longer needs `dev.license()` and an upload to get its key. The key is checked by the device, saved on it and applied after a restart (`PUT /fn/api/v1/license`, admin). A key set in the sketch with `dev.license()` takes precedence.
- The key typed in the setup wizard (*I have a key*) is the license key and the Flyght Cloud key; the dashboard offers the restart that applies it.
- Removing the saved key (Settings > Flyght Cloud, or `PUT /license` with an empty key) asks for the device name and is for the owner only.
- While the evaluation is over, sign-in and the key route still answer; every other route keeps answering 402.

## 2.1.0 — 2026-10-13

- **Renamed: FlyghtNinno is now FlyghtEdge.** The library name, the header and the precompiled archive
  (`libFlyghtEdge.a`) change; the `fn::` namespace, `FN_*` macros, settings stored on the device, MQTT
  topics, the license key format and existing keys do not. To migrate: remove the old `FlyghtNinno`
  library folder, install FlyghtEdge, and change `#include <FlyghtNinno.h>` to `#include <FlyghtEdge.h>`.
  Old sketches still compile through a deprecated `FlyghtNinno.h` that prints a note; it goes away in 3.0.
  The `User-Agent` of outgoing webhooks and flow HTTP requests is now `FlyghtEdge/<version>`.
- **Fifteen new examples** (10-24): GPS tracker and geofence, TFT status, SD logger, LCD, IMU tilt, MCU bridge, dashboard tour, Home Assistant, webhook alerts, flow rules, Flyght Cloud claim, and three projects (greenhouse, tank pump, power monitor). Each says in its header whether it has run on a board.
- **Guides**: fourteen English guides in `docs/guides` with wiring diagrams and GPIO maps (generated from the sketches and the firmware pin rules), desktop screenshots, a **screen catalog** with the desktop and the phone view of every screen, and a link to the Flyght Cloud dashboard on every page. The READMEs end with how to get in touch to contribute, for commercial use and to donate.
- **Relays, sensors and motors** with pins an admin can move from the card menu (three dots > Settings): `fn::Relay` (1-16 channels, active-high / active-low / latching, pulse, maximum on-time, interlock groups, rest time), `fn::Solenoid`, sensors `Dht`, `Ds18b20`, `Ultrasonic` (with tank level), `AnalogSensor` (water level, O2, H2, MQ gas, ACS712), `DigitalSensor`, `Mhz19`, `Pzem`, `Ina219`, and motors `DcMotor`, `Esc`, `Stepper`. The dialog offers only pins that can work for each role, a device that switches a load asks for the device name, GPIO pins apply at once and UART / I2C pins from the next start. New routes `GET /devices` and `PUT /devices/<id>/pins` (admin). Examples 07-09.
- **First-run wizard with two paths**, like an OS installer: **I have a key** (password, key, Wi-Fi) and **Set up online** (password, Wi-Fi, then the link that creates a key on the Flyght website). The key typed here is the Flyght Cloud key and can be skipped; the firmware edition still comes from `dev.license()`. The status of Flyght Cloud shows on the last step.
- **Dashboard: Flyght Cloud card** (Settings, admin): status, the link that claims the device, an on/off switch, the license key (write-only) and the server address. The saved key can be removed after typing the device name; the firmware checks the same name, so `PUT /cloud` with an empty `apiKey` and no `confirm` is refused (422). The card hides itself on a firmware without the cloud module.
- **Dashboard logo**: the FlyghtEdge mark replaces the "F" glyph. It is a 756-byte PNG inside the page (no SD card, no extra request); the White label feature shows the device initial instead.
- **Flyght Cloud** (`dev.cloud()`, off by default): claims the device with the license key, then mirrors the dashboard over MQTT to the broker the server names (certificate verified, reconnect with backoff). Admins get `GET/PUT /fn/api/v1/cloud`. The only address built in is the site; a device follows a server that moved.
- A widget id that Flyght Cloud would not accept (1-32 of `A-Z a-z 0-9 _ -`) is logged at startup.
- `Source::Cloud` for commands that arrive from the cloud.
- Visual flows: **Aperture** trigger block. Aperture (AI vision: people counting, PPE detection) posts a
  number to `POST /fn/api/v1/flows/aperture`; the flow starts on every change (or every update) for
  one feed and camera, with the number as `{value}`. Wire it to relays, buzzers, lights or widgets.

## 2.0.0 — 2026-10-09

License keys. **Breaking:** `dev.edition(...)` is gone; the edition comes from the key.

- License key (API_KEY) `fnk_live_…`: owner, edition, license id and update window, signed with
  ECDSA P-256 / SHA-256 and verified offline at boot with mbedTLS (hardware ECC on C5/C6). A
  CRC-32 checksum catches typos. Add it with `dev.license(FN_LICENSE_KEY)`; the examples read it
  from an optional `secrets.h`.
- Evaluation mode without a valid key: every feature on, a banner in the dashboard, and after
  30 minutes the app stops (HTTP 402) until the next reset. The boot log says why a key was rejected.
- *Settings → Plan & features* shows who the device is licensed to and the update window.
- HTTP API tokens: `fn_` + 256 random bits in base62 + checksum; tokens made by 1.x still work.
- License text 1.1: a license key is required; bypassing the check or sharing a key is not allowed.

Migrating from 1.x: remove `dev.edition(...)`, add `dev.license(...)` with your key.

## 1.0.0 — 2026-10-04

First release.

- Core: `fn::App`, module contract, dependency ordering, event bus, scheduler, `post()` to the loop task, NVS store, cJSON wrapper, ring-buffer logs.
- Network: station Wi-Fi (8 saved networks, priority order with automatic failover, save-as-backup, password edit, backoff, AP-aware deferral, async scan, C5 2.4 GHz policy), SoftAP (always/fallback/off, watchdog, DHCP option 114), captive portal (DNS + OS probes, per-client "online" after sign-in), mDNS (`_flyght._tcp`, refresh).
- Web: esp_http_server wrapper, wildcard routes, CORS, gzip+ETag UI, WebSocket broadcast.
- Auth: setup wizard, PBKDF2 admin password, guest access (viewer/operator), HMAC sessions that survive reboot, rate limiting, API tokens, sign-out-everywhere.
- UI: 8 widget types, pages, drag-and-drop layout stored on device, light/dark, English + Bahasa Indonesia.
- Integrations: MQTT (state/set, LWT, retained, Home Assistant discovery), HTTP REST, signed webhooks.
- System: info, module health, logs, OTA upload, restart, factory reset, NTP/browser clock.
- Devices (opt-in modules with their own UI pages): BNO055 IMU and HD44780 LCD (I2C), SD card file manager and ST7789/ILI9341/GFX TFT with drawing tools (SPI/SDMMC), GPS map with Leaflet and external-MCU serial link (UART).
- UI plugin system: per-module pages served gzip'd from flash, WebSocket topic subscriptions.
- UI: soft "dribbble" redesign, friendly sign-in page with kaomoji and rotating quotes, kid-friendly controls, Montserrat headings embedded in flash (offline-safe) paired with the system UI font.
- Lights: `fn::Led` (digitalWrite or PWM) and `fn::RgbLed` (RGB_BUILTIN / WS2812 pixel or 3 PWM pins), colour picker, brightness, 12 non-blocking effects, state kept in NVS, HTTP + MQTT + WebSocket.
- Visual flows (Pro): node editor in the browser (palette, canvas, bezier wires, inspector, pan/zoom, live highlight, templates), triggers/logic/actions + device blocks, serial or parallel branches, optional FreeRTOS thread per flow (Enterprise), stored in NVS.
- Editions and feature switches (Free / Pro / Enterprise): guest access, Visual flows, Users & roles, white label and RTOS flows start off; *Settings → Plan & features*.
- Users & roles (Pro): up to 8 accounts with viewer / operator / admin rights, username on the sign-in page.
- Example `06_BuiltinLed`: tests lights, effects and flows with only the board's LED and BOOT button.
- Fix (found on real hardware): the web server now brings up the TCP/IP stack itself (`Network.begin()`), so starting before the Wi-Fi/AP modules no longer crashes at boot. Verified on an ESP32-S3 (4 MB flash, 2 MB PSRAM, USB-Serial/JTAG).
- HTTP handler limit raised to 128 (all devices + lights + flows need ~100).
- Flows fixes (found on hardware): the Light block decides on/off from the state before the block, so "colour + toggle" on a timer blinks instead of staying dark, and a colour means a steady colour; **Run** works for every flow (Run button triggers, otherwise each trigger once); timers below 50 ms are refused.
- Pin safety: modules claim their GPIOs (`fn::claimPin`); flows cannot use pins of an LED, a bus, the SPI flash or USB (checked when saving and when running).
- Light state is written to NVS only when it really changed (fewer slow flash writes).
- RGB colour order for addressable LEDs (`colorOrder(fn::ColorOrder::RGB)`, `order` in the lights API, selector on the Lights page): fixes red/green swapped on boards with RGB-order LEDs.
- Phone UI: the layout-editor bar wraps (text above the buttons); the flow editor zooms out to show the whole flow on narrow screens.
- Fix: *Log out* now always lands on the sign-in page (a late hashchange used to redraw the dashboard without a session).
- `extras/test/demo.mjs`: records a walkthrough video (`--mobile`: 1080×1920 phone version with touch indicator) of the whole UI (sign in → dashboard → every device page → lights → flows → settings → log out) with ffmpeg.
- Precompiled release package: public headers and one archive per chip, with the examples and the documentation; the core version the archives need is checked at compile time. Log output is bound from the sketch so the archives work with every USB/UART setting, and a copyright notice is shown by the firmware, `GET /sys` and every UI script.
- License: FlyghtNinno Community License 1.0 (free to build and ship firmware; no redistribution, modification for redistribution, reverse engineering or competing libraries). NOTICE lists third-party components.
- Docs: new README (English) and README.id.md (Bahasa Indonesia), docs/REFERENCE.md (every class and function), docs/ARCHITECTURE.md (block diagrams), release guide in docs/MODULES.md.
- Tooling: UI build, device simulator (incl. all devices, lights and the flow engine), host unit tests, browser E2E tests.
