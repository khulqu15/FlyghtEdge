# Troubleshooting

Find the symptom, try the first cause, then the next. Each entry names the guide that explains the
part in full.

## The device

| Symptom | Likely cause | What to do |
|---|---|---|
| The setup Wi-Fi does not appear | the board does not start, or the access point is turned off | open the Serial Monitor at 115200 baud and look for the start-up lines; check *Wi-Fi → Access point* if it ran before |
| The page does not open by itself | the phone keeps the old Wi-Fi because it has no internet | open `http://192.168.4.1` yourself |
| `smart-lamp-xxxx.local` does not open | mDNS is not supported on that device | use the IP address from *Wi-Fi* or the wizard |
| The dashboard is slow | a long `delay()` or a blocking loop in the sketch | keep `loop()` short; the log warns when one pass takes more than 50 ms |
| Everything stops after 30 minutes | there is no license key, so evaluation mode ends | add `dev.license("fnk_live_...")` |
| Settings are lost after an upload | the partition scheme changed | use *No FS 4MB (2MB APP x2)* and keep it the same |
| Sketch too big | the partition has less room | pick the partition scheme above; build only the modules you use |

## Parts

| Part | Symptom | Likely cause |
|---|---|---|
| GPS | no satellites | TX/RX crossed, wrong baud rate, no sky view; see [GPS](04-gps.md) |
| TFT | blank screen | backlight pin, CS/DC wrong, RES not connected; see [TFT](05-tft.md) |
| SD card | not mounted | not FAT32, CS wrong, bad module; see [SD card](06-sd-card.md) |
| LCD | blocks only or blank | contrast screw, 5 V supply, address; see [LCD and IMU](07-lcd-and-imu.md) |
| IMU | "not ready" | SDA/SCL wrong, address 0x28/0x29, 3.3 V supply |
| MCU link | no values | baud rate, crossed lines, a 5 V TX without a divider; see [MCU link](08-mcu-link.md) |
| Relay | clicks at boot | the pin floats while the chip starts: use a relay board with a pull-up, or a different pin |
| Relay | does nothing | a missing ground, no supply on the board's own VCC, or an active-low board set as active-high |
| Sensor | shows "—" | the reading failed (CRC, no answer, out of range): check wiring and the pull-up |
| Motor | does not turn | separate supply, ground, enable pin; the card shows 0 % after a maximum run time |
| ESC | beeps and ignores the throttle | it is still arming; wait for the idle pulse to finish |

## Integrations

| Symptom | Likely cause |
|---|---|
| MQTT does not connect | wrong URI, user or password; use the *Test* button in *Connect → MQTT* |
| A card does not appear in Home Assistant | discovery is off, or the broker prefix differs from `homeassistant` |
| The webhook returns an error | the target must answer `2xx`; check the signature with the raw body |
| Flyght Cloud shows *Needs a license key* | add the key in the sketch or in *Settings → Flyght Cloud* |
| Flyght Cloud shows *Retrying* | the device cannot reach the site; check the Wi-Fi and the clock |

## Moving a pin

| Message | Meaning |
|---|---|
| `is used by the SPI flash` / `the USB port` | never usable; pick another pin |
| `is input-only` | an output cannot use that pin (GPIO 34–39 on the original ESP32) |
| `has no ADC that works with Wi-Fi` | an analog input needs an ADC1 pin |
| `is already used by another channel or device` | another module owns it; free it or choose another |
| `type the device name in "confirm"` | a device that switches a load asks for the device name |

## Start again

*Settings → Maintenance → Factory reset* erases the settings, the saved networks and the password.
For a problem in your own code, the log under *Settings* lists what each module is doing, and
`/fn/api/v1/sys/health` reports every module as ok, degraded, failed or disabled.

<!-- flyght-cloud-footer:start -->
---

**Flyght Cloud.** See this device next to your others, from anywhere: [flyght.obayan.id](https://flyght.obayan.id). Sign in, claim the device with its link, and the dashboard you built here is also on the web.
<!-- flyght-cloud-footer:end -->
