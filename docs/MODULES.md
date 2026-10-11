# Modules

Every FlyghtEdge feature is an `fn::Module`. Modules are registered with `app.use(...)`, started in dependency order, and run cooperatively inside `app.loop()`.

```
fn::App ── Bus (events) · Scheduler · post() · Guard (lock) · identity
   │
   ├── web      HTTP + WebSocket (esp_http_server)      deps: —
   ├── auth     login, guest, sessions, API tokens      deps: web
   ├── sys      info, health, logs, OTA, time           deps: web
   ├── ap       SoftAP + watchdog + DHCP opt 114        deps: ?web
   ├── captive  DNS catch-all + probe redirects         deps: ap, web
   ├── wifi     station, priorities, backoff, scan      deps: ?web, ?ap
   ├── mdns     <host>.local + _flyght._tcp             deps: ?web
   ├── ui       widgets, layout, /state                 deps: web, ?auth
   ├── mqtt     esp-mqtt bridge + HA discovery          deps: ui, ?web
   ├── webhook  outbound HTTP                           deps: ui, ?web
   │
   │   optional devices (src/fn/dev/, each brings its own UI plugin page)
   ├── imu      BNO055 over I2C                         deps: ?web
   ├── lcd      HD44780 + PCF8574 over I2C              deps: ?web
   ├── sd       SD card over SPI / SDMMC                deps: ?web
   ├── tft      ST7789 / ILI9341 / any GFX over SPI     deps: ?web
   ├── gps      NMEA receiver over UART                 deps: ?web
   └── mcu      line protocol to another MCU over UART  deps: ?web, ?ui
```

### UI plugins

A module can add a page: put `extras/ui/plugins/<id>.js` next to the others. `npm run build` minifies and gzips it into `src/fn/dev/ui/<id>.h`, and the module registers it with `web.plugin(id, title, icon, FN_P_<ID>, FN_P_<ID>_LEN, FN_P_<ID>_ETAG)`. The script calls `FN.register({ id, render(root, ctx) { ... } })` and can use the shared helpers (`FN.h`, `FN.api`, `FN.card`, `FN.toast`, ...). `ctx.on(fn)` receives live data that the module sends with `web.publish(id, json)`.

| Layer | Files |
|---|---|
| Pure logic, host-testable | `src/fn/pure/Pure.h` (backoff, rate limiter, tokens, templates, parsing) |
| Core | `src/fn/core/` (App, Bus, Scheduler, Kv (NVS), Json (cJSON), Log) |
| Modules | `src/fn/{web,auth,net,ui,mqtt,hook,sys}/` |
| Preset | `src/fn/Device.h` |

## Writing a module

```cpp
class Buzzer : public fn::Module {
public:
  const char* id() const override { return "buzzer"; }     // also the NVS namespace suffix and API path
  const char* deps() const override { return "web,?mqtt"; } // '?' = optional

  bool begin(fn::App& app) override {
    auto* web = app.get<fn::Web>("web");
    web->post("/fn/api/v1/buzzer", fn::Role::Operator, [this](fn::Request& q, fn::Response& s) {
      fn::Json b = q.json();
      uint32_t ms = (uint32_t)b.num("ms", 200);
      this->app().post([this, ms] { beep(ms); });  // HTTP runs on another task; hop to loop
      s.ok();
    });
    return true;                                 // false = module failed, app keeps running
  }
  void loop(uint32_t now) override { /* non-blocking */ }
  void onEvent(const fn::Event& e) override { if (e.id == fn::ev::MqttConnected) { ... } }
  fn::Health health() const override { return fn::Health::ok("idle"); }
};
```

### Rules

* **Threading.** `begin`, `loop`, `onEvent`, scheduler callbacks and widget callbacks run on the Arduino loop task. HTTP handlers, esp-mqtt and Wi-Fi events run on other tasks, so use `app().post(fn)` to move work onto the loop task, and `fn::App::Guard` for data shared with HTTP handlers.
* **Non-blocking.** Keep `loop()` short; the App logs a warning when a module takes more than 50 ms. Run slow I/O on your own task (see `Webhook`).
* **Settings.** Use `fn::Kv kv_{"fn.<id>"}`. NVS namespace and key names are limited to 15 characters. Code defaults should apply on first boot only, so that changes made in the UI win afterwards.
* **Routes.** Use `/fn/api/v1/<id>`. Register specific paths before wildcards (`/x/*` sets `q.tail`).
* **Events.** Built-in ids are in `fn::ev`. Use `fn::ev::User + n` for your own: `app().bus().emit(id, i, s)`.
* **Health.** Return `ok` / `degraded` / `failed` / `disabled` with a short message; it appears on the System page and in `/sys/health`.

## Tests

* `extras/test/host_tests.cpp`: unit tests for `Pure.h` / `Pure.cpp` (g++, no hardware).
* `extras/sim/server.mjs`: device simulator implementing the same API.
* `extras/test/e2e.mjs`: browser end-to-end test against the simulator, with a real MQTT broker and webhook receiver.
* Compile check: `arduino-cli compile --fqbn esp32:esp32:<chip>:PartitionScheme=no_fs examples/<name>`.

```bash
cd extras
npm install
npm run build     # web UI → gzip'd headers in src/
npm run sim       # device simulator on http://localhost:8080 (no ESP32 needed)
npm test          # browser end-to-end test + screenshots in extras/test/out
g++ -std=c++17 -I../src test/host_tests.cpp ../src/fn/pure/Pure.cpp -o host_tests && ./host_tests
```

## The precompiled package

The package contains the public headers, one precompiled archive per chip in `src/<mcu>/`,
the examples and the documentation. The Arduino IDE and arduino-cli pick the archive that
matches the board you select (`precompiled=full` in `library.properties`).

* Everything reachable from `FlyghtEdge.h` is the public API. Code that depends on board
  settings (`Serial`, `SDA`, `LED_BUILTIN`, `RGB_BUILTIN`...) lives in headers, so it is
  compiled in your sketch with your board's settings.
* The archives match the ESP32 core major.minor they were built for; `FlyghtEdge.h` stops
  the build with a clear message on another core. See the release notes for the core version.

## Flow blocks from your own module

```cpp
bool Buzzer::begin(fn::App& app) {
  fn::flowNode({"buzzer.beep", "device", "Beep", "bell", "Short beep on the buzzer.", 1, {"then"},
                {{"ms", "Length (ms)", "number", "200", ""}},
                [this](fn::FlowCtx& c) { beep(c.num("ms", 200)); return fn::NodeResult::next(); }});
  return true;
}
```

* `run` is called on the loop task (also for flows on an RTOS thread), so it may use any module.
* Return `NodeResult::next(out)` to follow an output, `NodeResult::wait(ms)` to pause without
  blocking, `NodeResult::stop()` to end the branch. For slow work set `async = true`, do it on
  your own task and call `c.resume(out, value)` when done.
* Parameter kinds: `text`, `number`, `select` (`opts` = `"a|b|c"`), `bool`, `color`, `pin`,
  `widget`, `light`, `effect`.

## Feature switches

```cpp
app.features().define("myReports", "Monthly reports", fn::Edition::Pro, false, "Email a report every month.");
if (app.features().on("myReports")) { ... }
```

Switches appear in *Settings → Plan & features*, with a lock and the edition they need when the
current edition is too low.
