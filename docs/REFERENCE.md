# C++ reference

Everything you call from a sketch, grouped by class. All names live in namespace `fn`.
Builder methods (`Foo& x(...)`) return the object, so they can be chained; call them **before**
`dev.begin()`. Values set by builders are *first-boot defaults*: once a setting is changed in the
web UI, the stored value wins.

> Thread rule: call module methods from `setup()` / `loop()` (the loop task). From other tasks or
> ISRs use `app.post([]{ ... })` to run code on the loop task.

**Contents** —
[Device](#device) · [App](#app) · [Module](#module) · [Events, scheduler, health](#events-scheduler-health) ·
[Page & widgets](#page--widgets) · [Ui](#ui) · [Auth](#auth) · [Wifi](#wifi) · [Ap](#ap) · [Captive](#captive) ·
[Mdns](#mdns) · [Web](#web) · [Mqtt](#mqtt) · [Cloud](#cloud) · [Webhook](#webhook) · [System](#system) ·
[Features & editions](#features--editions) · [Light / Led / RgbLed](#light--led--rgbled) · [Relays, sensors, motors](#relays-sensors-motors) · [Flows](#flows) ·
[Flow blocks API](#flow-blocks-api) · [Devices](#devices) · [Json](#json) · [Kv (NVS)](#kv-nvs) · [Logging](#logging)

---

## Device

`fn::Device` is an `App` with the standard modules already added.

```cpp
fn::Device dev("smart-lamp", "1.2.0");   // product name (slug) and your firmware version
```

| Member | Returns | Description |
|---|---|---|
| `Device(name, version = "1.0.0")` | | `name` is used for the AP name, hostname and MQTT topics |
| `page(name, icon = "layout-grid")` | `Page&` | Add a dashboard page (same as `ui().page`) |
| `web()` `auth()` `wifi()` `ap()` `captive()` `mdns()` `ui()` `mqtt()` `webhook()` `system()` | module `&` | Access a built-in module to configure it |
| `use(module)` | `App&` | Add your own or a device module |
| `begin()` | | Add the built-in modules, then start everything (call last in `setup()`) |
| `loop()` | | Run all modules (call in `loop()`) |
| everything from [App](#app) | | |

Built-in modules and their start order:

| id | Class | Depends on (`?` = optional) |
|---|---|---|
| `web` | `Web` | — |
| `auth` | `Auth` | web |
| `sys` | `System` | web |
| `ap` | `Ap` | ?web |
| `captive` | `Captive` | ap, web |
| `wifi` | `Wifi` | ?web, ?ap |
| `mdns` | `Mdns` | ?web |
| `ui` | `Ui` | web, ?auth |
| `mqtt` | `Mqtt` | ui, ?web |
| `webhook` | `Webhook` | ui, ?web |

## App

```cpp
fn::App app("field-meter");
app.use(web).use(auth).use(ui);
app.begin();
```

| Member | Returns | Description |
|---|---|---|
| `App(name, version = "1.0.0")` | | Create the container (binds log output to `Serial`) |
| `use(Module&)` | `App&` | Add a module; order does not matter, dependencies decide |
| `begin()` | | Sort modules by `deps()`, call each `begin()`, emit `ev::AppReady` |
| `loop()` | | Run posted jobs, the scheduler and each module's `loop()` |
| `find(id)` | `Module*` | Module by id, or `nullptr` |
| `get<T>(id)` | `T*` | Typed module, only if it is running |
| `modules()` | `const vector<Module*>&` | All modules |
| `bus()` | `Bus&` | Event bus (see [Events](#events-scheduler-health)) |
| `sched()` | `Scheduler&` | Timers on the loop task |
| `features()` | `Features&` | Edition switches (see [Features](#features--editions)) |
| `license(key)` | `App&` | License key (`"fnk_live_..."`), before `begin()`. Sets the edition; without a valid key: evaluation mode, 30 min per boot. `edition(Edition)` was removed in 2.0 |
| `post(fn)` | `bool` | Run `fn` on the loop task; safe from any task |
| `App::Guard g;` | | Global lock for state shared with the HTTP task |
| `name()` `version()` | `const String&` | As given to the constructor |
| `deviceId()` | `const String&` | 12 hex digits from the factory MAC |
| `displayName()` / `setDisplayName(s)` | | Name shown in the UI (user editable) |
| `hostname()` / `setHostname(s)` | | mDNS / DHCP hostname |
| `expand(tpl)` | `String` | Replaces `{name} {id} {mac4} {mac6} {version} {chip}` |
| `restart(reason, delayMs = 800)` | | Restart after the HTTP reply was sent |
| `factoryReset()` | | Erase all FlyghtEdge settings and restart |
| `uptimeSec()` | `uint32_t` | Seconds since boot |
| `setYield(ms)` | | Delay at the end of each `loop()` (default 1 ms) |

## Module

Base class of every feature. Override what you need.

| Member | Required | Description |
|---|:-:|---|
| `const char* id() const` | ✔ | Unique id, also used in URLs and logs |
| `bool begin(App&)` | ✔ | Start; return `false` to mark the module failed (others keep running) |
| `const char* title() const` | | Human name (default: id) |
| `const char* deps() const` | | `"web,?mqtt"` — comma separated ids; `?` = optional |
| `void loop(uint32_t nowMs)` | | Called every `app.loop()`; keep it under ~50 ms |
| `void onEvent(const Event&)` | | Every bus event |
| `Health health() const` | | Shown on the System page |
| `void describe(JsonRef out) const` | | Extra details for `/sys/health` |
| `bool running() const` | | `begin()` succeeded |
| `App& app() const` | | The container |

## Events, scheduler, health

```cpp
dev.bus().on(fn::ev::StaGotIp, [](const fn::Event& e) { Serial.println(e.s); });
dev.sched().every(5000, [] { /* every 5 s on the loop task */ });
```

| Event (`fn::ev::`) | `i` | `s` |
|---|---|---|
| `AppReady` | | |
| `StaConnected` / `StaGotIp` | | SSID / IP |
| `StaDisconnected` | reason code | reason text |
| `ApStarted` / `ApStopped` | | IP |
| `ApClientJoin` / `ApClientLeave` | client count | client IP |
| `MdnsReady` | | hostname |
| `MqttConnected` / `MqttDisconnected` | | |
| `WidgetChanged` | | widget id |
| `ConfigChanged` | | module id |
| `Login` | role | client IP |
| `TimeSynced` | | |
| `User` (1000+) | yours | yours |

| `Bus` | Description |
|---|---|
| `on(id, fn)` | Subscribe (`ev::Any` = all) |
| `emit(id, i = 0, s = "")` | Publish (loop task only) |

| `Scheduler` | Description |
|---|---|
| `every(ms, fn, runNow = false)` | Repeat; returns an id |
| `after(ms, fn)` | Once |
| `cancel(id)` | Stop |

`Health`: `Health::ok(msg)`, `degraded(msg)`, `failed(msg)`, `disabled(msg)`.
Roles (`fn::Role::`): `None 0`, `Viewer 1`, `Operator 2`, `Admin 3`. Clock: `timeValid()`, `unixTime()`.

## Page & widgets

```cpp
dev.page("Kitchen", "home")
   .toggle("light", "Light", [](bool on) { ... }).icon("bulb")
   .slider("dim", "Dimmer", 0, 100, 5, [](float v) { ... }).unit("%").size(2);
```

| Method | Value type | Notes |
|---|---|---|
| `toggle(id, label, void(bool) cb = nullptr)` | bool | |
| `button(id, label, void() cb)` | — | |
| `slider(id, label, min, max, step = 1, void(float) cb = nullptr)` | number | value snaps to `step` |
| `select(id, label, {"a","b"}, void(const String&) cb = nullptr)` | text | only listed options accepted |
| `text(id, label, void(const String&) cb = nullptr)` | text | max 128 characters |
| `gauge(id, label, unit, min, max)` | number | read-only |
| `value(id, label, unit = "")` | number or text | read-only |
| `chart(id, label, unit = "", points = 60)` | number history | read-only |

| Modifier (last widget) | Effect |
|---|---|
| `size(1..3)` | Grid columns: 1 small, 2 wide, 3 full row |
| `icon(name)` | `home layout-grid bulb fan thermo droplet zap sun gauge activity chart sliders list type pointer bell power lock wifi plug cpu key shield` … |
| `unit(u)` | Unit text |
| `decimals(n)` | Digits after the point |
| `role(minRole)` | Minimum role to change it (default `Role::Operator`) |
| `every(ms)` | Chart sampling period |

Callbacks run on the loop task, also when the change came from MQTT, HTTP or a flow.

## Ui

| Member | Description |
|---|---|
| `page(name, icon)` | Add a page |
| `set(id, value)` | Set a widget (number, bool, text); pushed to browsers, MQTT, webhooks. Loop task |
| `number(id)` / `flag(id)` / `text(id)` | Read the current value |
| `apply(id, raw, Source)` | Parse raw input like `"on"`, `"42"`, run the callback (loop task) |
| `listen(void(const Widget&, Source) fn)` | Called after every change |
| `find(id)` | `Widget*` or `nullptr` |
| `accent(css)` | Accent colour, e.g. `"#16a34a"` |
| `pushInterval(ms)` | How often changes are pushed (default 150 ms) |

`Source`: `Code`, `Web`, `Mqtt`, `Http`.

## Auth

| Member | Description |
|---|---|
| `defaultPassword(pw)` | Skip the setup wizard by setting the admin password in code |
| `allowAnonymous(on, role = Role::Viewer)` | Guest access default (feature `guest`); role `Viewer` or `Operator` |
| `sessionDays(days)` | How long "remember me" sessions last (default 30) |
| `setupRequired()` | `true` until a password exists |
| `guestAllowed()` / `rbacOn()` | Current switches |

Sessions are HMAC-SHA256 signed tokens (cookie `fn_s` or `Authorization: Bearer`), valid after
reboots; *Sign out everywhere* revokes all of them. Five wrong passwords lock login for 30 s and
the lock doubles on further failures.

## Wifi

| Member | Description |
|---|---|
| `add(ssid, pass, priority = 50)` | Default network (up to 8 kept; higher priority first) |
| `connectTimeout(sec)` | Per attempt (default 15 s) |
| `backoff(baseSec, maxSec)` | Retry delay after failures |
| `connected()` / `ssid()` | Station state |
| `connectNow(ssid, pass)` | Connect at once (loop task) |
| `reconnect()` | Retry now |

## Ap

| Member | Description |
|---|---|
| `ssid(tpl)` | Default `"{name}-{mac4}"`; templates from `App::expand` |
| `password(pw)` | `""` = open; WPA2 needs ≥ 8 characters |
| `mode(ApMode)` | `Always` (default), `Fallback` (only while Wi-Fi is down), `Off` |
| `ip(IPAddress)` | Default `192.168.4.1` |
| `channel(ch)` / `maxClients(n)` | Radio settings |
| `active()` / `address()` / `ssidNow()` | State |

## Captive

| Member | Description |
|---|---|
| `mode(CaptiveMode)` | `UntilLogin` (default: a phone that signed in is told "internet OK" so it stays connected) or `Always` |

## Mdns

| Member | Description |
|---|---|
| `service(name, proto, port)` | Extra DNS-SD service (`_flyght._tcp` is always advertised) |
| `refreshEvery(minutes)` | Re-announce period |
| `restart(reason)` / `ready()` | Control and state |

## Web

| Member | Description |
|---|---|
| `Web(port = 80)` | |
| `get/post/put/del(uri, minRole, handler)` | Route; a trailing `*` matches the rest (`q.tail`) |
| `plugin(id, title, icon, gz, len, etag)` | Add a UI page (gzip'd JS served from flash) |
| `wanted(topic)` / `publish(topic, json)` | WebSocket topics, sent only while a page subscribes |
| `broadcast(text)` / `send(fd, text)` | Raw WebSocket messages (thread-safe) |

Handlers run on the HTTP task. `Request`: `path()`, `query(k)`, `header(k)`, `cookie(k)`,
`clientIp()`, `body(max)`, `json(max)`, `tail`, `role`, `user`. `Response`: `json(code, j)`,
`text(code, s)`, `ok()`, `error(code, msg)`, `redirect(url)`, `header(k, v)`, `beginStream/chunk/endStream`.

```cpp
dev.web().get("/fn/api/v1/hello", fn::Role::Viewer, [](fn::Request& q, fn::Response& s) {
  fn::Json j;  j.set("hello", q.user);  s.json(j);
});
```

## Mqtt

| Member | Description |
|---|---|
| `broker(uri)` | `mqtt://host:1883`, `mqtts://…`, `ws://…` |
| `credentials(user, pass)` | |
| `baseTopic(tpl)` | Default `flyght/{id}` |
| `homeAssistant(on)` | Publish discovery for every widget |
| `connected()` / `base()` | State |
| `publish(topic, payload, retain = false, qos = 0)` | `bool` |
| `subscribe(topic, void(topic, payload) cb)` | Callback on the loop task |

## Cloud

| Member | Description |
|---|---|
| `enable(on = true)` | Code default for the first boot; the dashboard setting wins afterwards. Off by default |
| `site(url)` | `https://host[:port][/path]`; a saved address wins over this one |
| `apiKey(key)` | Key used to register; default is the one given to `license()` |
| `state()` | `Off`, `NoNetwork`, `NeedKey`, `Claiming`, `Fetching`, `Connecting`, `Online`, `Retry`, `Stopped` |
| `connected()` | `true` while online |
| `site()` | The address in use now |
| `activateUrl()` | Link that claims the device in the web account |

HTTP: `GET/PUT /fn/api/v1/cloud` (admin), see [API.md](API.md). The key is never returned.

## Webhook

| Member | Description |
|---|---|
| `url(u)` / `secret(s)` | Target and HMAC-SHA256 key (`X-Flyght-Signature`) |
| `interval(sec)` | Also send all values periodically (0 = only on change) |
| `caCert(pem)` | Verify an HTTPS server |
| `send(event, extra = {})` | Queue a custom event |

## System

| Member | Description |
|---|---|
| `ntp(server1, server2)` | Time servers |
| `timezone(posixTz)` | e.g. `"WIB-7"`, `"CET-1CEST,M3.5.0,M10.5.0/3"` |

## Features & editions

```cpp
dev.license(FN_LICENSE_KEY);   // a Pro key: the edition comes from the key
dev.features().defaultOn("flows");
dev.features().define("reports", "Monthly reports", fn::Edition::Pro, false, "Email a report every month.");
if (dev.features().on("reports")) { ... }
```

| Member | Description |
|---|---|
| `define(id, title, minEdition, defaultOn, desc)` | Register a switch (first definition wins) |
| `defaultOn(id, on = true)` | First-boot default |
| `allowed(id)` | Edition is high enough |
| `on(id)` | Allowed **and** switched on (cached; cheap to call often) |
| `set(id, on)` | Persist; `false` when not allowed |
| `edition()` | Current edition (from the license key) |

Built-in ids: `guest`, `apiTokens`, `flows`, `rbac`, `whitelabel`, `flowsRtos` (see README).

## Light / Led / RgbLed

```cpp
fn::Led led(2, "status", /*activeHigh*/ true, /*pwm*/ true);
fn::RgbLed pixel(RGB_BUILTIN, "builtin");
fn::RgbLed strip(5, 6, 7, "desk", /*commonAnode*/ false);
```

| Constructor | Description |
|---|---|
| `Led(pin, id = "led", activeHigh = true, pwm = true)` | Single colour; `pwm=false` uses `digitalWrite` (on when brightness ≥ 25 %) |
| `RgbLed(dataPin, id = "rgb")` | One addressable pixel (WS2812, `RGB_BUILTIN`) |
| `RgbLed(r, g, b, id = "rgb", commonAnode = false)` | Three PWM pins |

| Member | Returns | Description |
|---|---|---|
| `on(bool = true)` / `off()` / `toggle()` | `Light&` | Power |
| `color(0xRRGGBB)` / `color(r, g, b)` | `Light&` | Colour (turns the light on; ends colour-less effects) |
| `brightness(0..100)` | `Light&` | Percent, gamma corrected |
| `effect(Fx, periodMs = 0, color2 = keep)` | `Light&` | Start an effect; `0` = default speed |
| `effect("rainbow", periodMs = 0)` | `bool` | By id; `false` if unknown |
| `apply(json, &err)` | `bool` | `{on, color, color2, brightness, effect, period}` |
| `state()` | `const State&` | `on, color, color2, brightness, fx, period` |
| `isRgb()` | `bool` | |
| `colorOrder(ColorOrder)` / `colorOrder()` | `Light&` / order | Addressable LEDs: byte order `GRB` (default, WS2812B), `RGB`, `BRG`, `RBG`, `GBR`, `BGR`. Pick `RGB` when red and green look swapped. Also on the Lights page (admin) |
| `Light::all()` | list | Every light |

`fn::Fx`: `Solid Blink Breathe Heartbeat Strobe Rainbow Cycle Candle Police Sos Sunrise Alarm`.

## Relays, sensors, motors

All of them are modules (`dev.use(x)`), linked only when created. `id` and `title` must outlive the object;
`id` is at most 15 characters (it is the key of the saved pins).

**Relay** — `fn::Relay relays("relay", "Title");`

| Member | Description |
|---|---|
| `channel(pin, name, type)` | Add a channel (1-16). `Type::ActiveHigh` (default), `ActiveLow`; pin -1 = not wired yet |
| `latching(setPin, resetPin, name)` | Bistable relay: a 50 ms pulse on the set or reset pin |
| `bootOn(on)` · `maxOn(seconds)` · `pulse(ms)` · `interlock(group)` · `minOff(seconds)` | Settings of the channel added last |
| `page(name)` | Dashboard page (default "Relays") |
| `set(channel, on)` | `false` when refused: no pin, interlock busy, still resting |
| `isOn(channel)` · `channels()` | State |

`fn::Solenoid valve(pin, id, name, maxOnSeconds = 60, minOffSeconds = 2)` is a one-channel relay with those two limits.

**Sensors** (`#include <FlyghtEdge.h>`) — common to all: `every(ms)`, `staleAfter(ms)`, `page(name)`, `alarm(high, low, cb)`, `value(i)`.

| Class | Constructor | Notes |
|---|---|---|
| `Dht` | `Dht(pin, Type::Dht22, id)` | `temperature()`, `humidity()`; at least 2 s (DHT11 1 s) between reads |
| `Ds18b20` | `Ds18b20(pin, id)` | one sensor; `temperature()` |
| `Ultrasonic` | `Ultrasonic(trig, echo, id)` | `airTemperature(c)`, `tank(emptyCm, fullCm)`, `distanceCm()` |
| `AnalogSensor` | `AnalogSensor(adc1Pin, id, title, unit)` | `map(mvLo, mvHi, outLo, outHi)`, `range(lo, hi, dec)`, `hall(zeroMv, mvPerAmp)`, `mq(Vc, RL, R0, a, b)`, `millivolts()` |
| `DigitalSensor` | `DigitalSensor(pin, id, title)` | `activeLow()`, `pullUp()`, `labels(active, idle)`, `debounce(ms)`, `onChange(cb)`, `active()` |
| `Mhz19` | `Mhz19(rx, tx, port = FN_SPARE_UART, id)` | `ppm()` |
| `Pzem` | `Pzem(rx, tx, port = FN_SPARE_UART, address = 0xF8, id)` | `volts()`, `amps()`, `watts()` (and energy, frequency, power factor cards) |
| `Ina219` | `Ina219(address = 0x40, shuntOhm = 0.1, wire = Wire, sda = -1, scl = -1, id)` | `volts()`, `amps()`, `watts()` |

**Motors**

| Class | Constructor | Members |
|---|---|---|
| `DcMotor` | `DcMotor(pwm, in1, in2 = -1, id, title)` | `ramp(percentPerSec)`, `maxRun(seconds)`, `pwmFrequency(hz)`, `speed(percent)`, `stop()` |
| `Esc` | `Esc(pin, id, title)` | `pulse(minUs, maxUs)`, `ramp(percentPerSec)`, `armTime(ms)`, `maxRun(seconds)`, `throttle(percent)`, `stop()`, `armed()` |
| `Stepper` | `Stepper(step, dir, enable = -1, id, title)` | `maxSpeed(sps)`, `acceleration(sps2)`, `limits(min, max)`, `enableActiveHigh()`, `releaseAfter(ms)`, `reverse()`, `moveTo(pos)`, `moveBy(n)`, `stop()`, `position()`, `setPosition(p)`, `moving()` |

**Pins of your own device.** Implement `fn::PinDevice` (`pinDeviceId`, `pinDeviceTitle`, `pinSlots`, `pinApply`; optional `pinsDriveLoad`, `pinsNeedRestart`), call `fn::pins::add(app, this)` in `begin()`, read saved pins with `fn::pins::saved(id, key, default)`, and put `.device(id)` on its widgets: the card menu then offers Settings.

## Flows

| Member | Description |
|---|---|
| `Flows flows; dev.use(flows);` | Adds the engine and the **Flows** page (needs feature `flows`) |
| `run(flowId, value = "")` | Start a flow: its Run button triggers, or each trigger once |
| `hook(name, value)` | Fire HTTP-hook triggers by name; returns how many |
| `kMaxFlows 8`, `kMaxNodes 40`, `kMaxWires 80`, `kMaxTokens 16` | Limits |

## Flow blocks API

Add blocks from any module (usually in `begin()`):

```cpp
fn::flowNode({"buzzer.beep", "device", "Beep", "bell", "Short beep on the buzzer.", 1, {"then"},
              {{"ms", "Length (ms)", "number", "200", ""}},
              [](fn::FlowCtx& c) { beep(c.num("ms", 200)); return fn::NodeResult::next(); }});
```

| `NodeDef` field | Description |
|---|---|
| `type` | Unique id, `"module.action"` |
| `cat` | `trigger`, `logic`, `action` or `device` (palette group and colour) |
| `title`, `icon`, `desc` | Shown in the editor |
| `inputs` | `1`, or `0` for triggers |
| `outputs` | Output labels, e.g. `{"yes", "no"}` |
| `params` | `{key, label, kind, default, options}`; kinds: `text number select bool color pin widget light effect` |
| `run` | `NodeResult(FlowCtx&)`, called on the loop task |

| `FlowCtx` | Description |
|---|---|
| `str(key)` | Parameter with `{value} {uptime} {time} {id} {name} {flow}` filled in |
| `num(key, def)` / `flag(key, def)` / `raw(key)` | Typed access |
| `value` | Value travelling on the wire (read and write) |
| `resume(out, value)` | Finish an async block later (any task) |

| `NodeResult` | Meaning |
|---|---|
| `next(out = 0)` | Follow an output |
| `wait(ms, out = 0)` | Pause without blocking, then follow |
| `stop()` | End this branch |
| `async = true` | Finish later through `resume` |

Pins: `claimPin(pin, owner)` marks a pin as used, so flows cannot drive it; `pinOwner(pin)`,
`pinProblem(pin, output)` check a pin.

## Devices

| Class | Constructor | Builders / methods |
|---|---|---|
| `Bno055` | `(addr = 0x28, TwoWire& = Wire)` | `pins(sda, scl)`, `rate(hz)`, `externalCrystal(on)`; `ready()`, `yaw()`, `pitch()`, `roll()`, `temperature()`, `level()`, `clearLevel()` |
| `LcdI2c` | `(cols = 16, rows = 2, addr = auto, TwoWire& = Wire)` | `pins(sda, scl)`; `clear()`, `print(row, col, text)`, `line(row, text)`, `backlight(on)`, `text(row)` |
| `SdCard` | `(cs = SS, SPIClass& = SPI)` | `pins(sck, miso, mosi)`, `frequency(hz)`, `mmc(oneBit, clk, cmd, d0)`; `mounted()`, `fs()`, `remount()`, `SdCard::Lock lock(sd);` |
| `TftSpi` | `(TftSpi::ST7789 \| ILI9341, w, h, cs, dc, rst = -1, SPIClass& = SPI, hz = 40 MHz)` | `pins(sck, mosi, miso = -1)` |
| `GfxTarget<G>` | `(G& gfx, bool() init = nullptr)` | Any Adafruit-GFX style display |
| `Tft` | `(TftTarget&)` | `backlight(pin, activeHigh)`; `clear(c)`, `text(x, y, s, c, size, bg)`, `rect(...)`, `line(...)`, `circle(...)`, `rotation(r)`, `brightness(%)`, `splash()`, `Tft::rgb(r, g, b)` |
| `Gps` | `(rx, tx = -1, baud = 9600, HardwareSerial& = Serial1)` | `track(maxPoints, minMeters)`, `setClock(on)`; `fix()`, `hasFix()`, `lat()`, `lon()`, `clearTrack()` |
| `McuLink` | `(rx, tx, baud = 115200, HardwareSerial& = Serial1)` | `bind(key, widget)`, `forward(widget, key)`, `quick(label, cmd)`, `onLine(cb)`, `lineEnding(eol)`; `send(line)`, `value(key)`, `online()` |

## Json

Thin wrapper over cJSON. `Json` owns a document; `JsonRef` is a view into it.

| Member | Description |
|---|---|
| `Json j;` / `Json::array()` / `Json::parse(text)` | Create; `ok()` after parse |
| `set(key, v)` | String, bool, number; chainable |
| `obj(key)` / `arr(key)` | Child object / array (`JsonRef`) |
| `pushObj()` / `pushArr()` / `push(v)` | Array items |
| `get(key)` / `at(i)` / `has(key)` / `size()` | Navigate |
| `str(key, def)` / `num(key, def)` / `flag(key, def)` | Read members |
| `isString()` `asString()` `isNumber()` `asNumber()` `isBool()` `asBool()` `key()` | Read this node |
| `each([](JsonRef v){...})` | Iterate members / items |
| `dump()` | Compact text |

## Kv (NVS)

```cpp
fn::Kv kv("myapp");             // NVS namespace (max 15 chars)
kv.putU32("count", kv.u32("count") + 1);
```

| Read | Write |
|---|---|
| `str(key, def)` | `put(key, String)` |
| `i32(key, def)` / `u32(key, def)` | `putI32` / `putU32` |
| `flag(key, def)` / `f32(key, def)` | `putFlag` / `putF32` |
| `bytes(key, buf, len)` / `blob(key)` | `putBytes(key, buf, len)` |
| `has(key)` | `remove(key)` / `clear()` |

## Logging

```cpp
FN_LOGI("app", "temperature %.1f", t);   // also FN_LOGE, FN_LOGW, FN_LOGD
fn::log::setLevel(fn::LogLevel::Debug);
```

Lines go to `Serial` and to the ring buffer shown on the System page (`GET /fn/api/v1/sys/logs`).
Identical consecutive lines are collapsed to `(repeated xN)`. `fn::log::setOutput(&Serial1)` sends
them elsewhere, `nullptr` keeps them only in the buffer.
