# HTTP & WebSocket API (v1)

Base URL: `http://<device>/fn/api/v1`. Every response is JSON and errors look like `{"error": "..."}`.

Without a valid license key the device runs in evaluation mode; once its 30 minutes are used up,
every route except `GET /auth/state` answers **402** until the next reset.

## Authentication

A request is authenticated by one of the following, checked in this order:

1. `Authorization: Bearer <token>`. The token is either an **API token** (`fn_…`, created in *Integrations → HTTP API*) or a session token.
2. The `fn_s` session cookie, set by `login`, `anonymous` and `setup`.
3. The `?t=<token>` query parameter (used by the WebSocket).

Roles: `viewer` (1) < `operator` (2) < `admin` (3). Session tokens are HMAC-SHA256 signed with a per-device secret kept in NVS, so they stay valid after reboots. `logout-all` revokes every session.

| Method | Path | Role | Body / notes |
|---|---|---|---|
| GET | `/auth/state` | none | `{setup, anonymous, anonRole, rbac, role, user, name, model, version, fn, id, timeValid, edition, license:{state, edition, evalMinutes?, evalLeft?, error?}, features:{id:on}}`. `state`: `licensed`, `evaluation` or `stopped` |
| POST | `/auth/setup` | none | First boot only: `{password, anonymous, anonRole, now}`. Guest access stays off unless `anonymous: true` |
| POST | `/auth/login` | none | `{username?, password, remember, now}`. No username (or `admin`) = the owner. Returns 429 `{retryAfter}` after 5 failed attempts |
| POST | `/auth/anonymous` | none | `{remember, now}`. Returns 403 when guest access is off |
| POST | `/auth/logout` | none | Clears the cookie |
| POST | `/auth/logout-all` | admin | Revokes every session |
| PUT | `/auth/password` | admin | `{current, next}` (owner only) |
| GET/PUT | `/auth/config` | admin | `{anonymous, anonRole, sessionDays}` |
| GET/POST | `/auth/tokens` | admin | POST `{name, role}` returns `{token}`, shown once: `fn_` + 43 base62 characters (256 random bits) + a 6-character checksum. Tokens made by 1.x keep working |
| DELETE | `/auth/tokens/<prefix>` | admin | |
| GET/POST | `/auth/users` | admin | Users & roles (Pro, `rbac` on). POST `{name, password, role}`; 403 on Free, 409 while switched off |
| PUT/DELETE | `/auth/users/<id>` | admin | PUT `{role?, password?}`. A role change applies to open sessions at once |
| GET | `/features` | viewer | `{edition, features:[{id, title, desc, min, allowed, on}], license:{state, edition, owner?, id?, issued?, updatesUntil?, evalLeft?, error?}}` |
| PUT | `/features` | admin | `{id, on}`. 403 `"<title> needs the <edition> edition"` when the edition is too low |

Session tokens carry the account: owner, guest, or a Users & roles account. Guest sessions stop
working when guest access is turned off, account sessions when Users & roles is turned off.

## Dashboard & values

| Method | Path | Role | Notes |
|---|---|---|---|
| GET | `/ui` | viewer | Pages, widgets (type, label, unit, min/max/step, options, value, chart history) and layout |
| GET | `/state` | viewer | `{values: {id: value}, ts, uptime}` |
| POST | `/state` | per widget | Batch update `{"light": true, "fan": 40}`. All values are validated before any is applied |
| POST | `/ui/w/<id>` | per widget | `{"value": …}`. `null` toggles a switch or presses a button |
| PUT | `/ui/layout` | admin | `{page, order[], sizes{id: 1-3}, hidden[]}` |
| DELETE | `/ui/layout?page=<id>` | admin | Resets one page, or all pages when `page` is omitted |

Value formats: toggle `true/false` (also `"on"`, `"off"`, `"toggle"`), slider a number (clamped to the widget's range and step), select one of the options, text up to 128 characters. Gauges, values and charts are read-only (422).

## Network

| Method | Path | Role | Notes |
|---|---|---|---|
| GET | `/wifi` | viewer | State, SSID, IP, RSSI, last error, retry time. Admins also get `networks[]` |
| PUT | `/wifi/networks` | admin | Replaces the whole list `{networks:[{ssid, priority, password?}]}` (max 8, unique SSIDs). Use it to delete, reorder or change a password. Removing the network in use moves on to the next one |
| POST | `/wifi/connect` | admin | `{ssid, password, connect}`: `connect:true` (default) saves it as choice #1 and connects now; `connect:false` saves it as the last backup and keeps the current connection. Returns 409 when 8 networks are saved |
| POST | `/wifi/reconnect` | admin | |
| POST/GET | `/wifi/scan` | admin | Starts an async scan / reads results. Returns 409 while a phone uses the AP, unless called with `?force=1` |
| GET/PUT | `/ap` | viewer/admin | `{ssid, password, mode: always\|fallback\|off, channel}` |
| GET/PUT | `/mdns` | viewer/admin | `{hostname}` |

## Integrations & system

| Method | Path | Role |
|---|---|---|
| GET/PUT | `/mqtt` · POST `/mqtt/test` | admin |
| GET/PUT | `/cloud` | admin | Flyght Cloud: enable, site, license key (never returned; sending an empty `apiKey` removes it and needs `"confirm"` = the device name, else 422); state and activation link |
| GET | `/devices` | admin | Devices whose pins can be moved: their pins (key, label, role, protocol, current, sketch default), whether a change needs a restart, and for each role the pins that can work (with `warn` for boot pins and `by` for who has a pin) |
| PUT | `/devices/<id>/pins` | admin | `{"pins":{"<key>":4,"<key>":null},"confirm":"<device name>"}`: 422 with the reason for a pin that cannot work (flash, USB, input-only, not ADC1, already used, nonexistent) and for a missing `confirm` on a device that switches a load; GPIO pins apply at once, `restart:true` in the answer means the pins are used from the next start |
| GET/PUT | `/webhook` · POST `/webhook/test` | admin |
| GET/PUT | `/sys` (PUT `{name}`) | viewer/admin |
| GET | `/sys/health`: every module's `{state: ok\|degraded\|failed\|disabled, msg, details}` | viewer |
| GET | `/sys/logs?since=<seq>` | admin |
| POST | `/sys/time` `{now}` (accepted only while the clock is unknown) | none |
| POST | `/sys/restart` · `/sys/factory-reset {"confirm":"RESET"}` | admin |
| POST | `/sys/ota`: raw `.bin` as the request body | admin |

## WebSocket `/fn/ws`

Authenticated with the session cookie or `?t=`. The server sends:

* `{"t":"v","v":{"temp":24.5,"light":true}}`: changed values, batched every 150 ms
* `{"t":"layout"}`: the layout changed; reload `/ui`
* `{"t":"sys","d":{uptime, heap, rssi, sta, ts}}`: every 5 s

## Webhook payload

```json
{"event":"change","device":"A4CF12B1C2D3","name":"Smart Lamp","model":"smart-lamp",
 "ts":1730000000,"uptime":1234,"values":{"light":true},"changed":["light"]}
```

`event` is `change`, `interval`, `test`, or your own name via `webhook.send()`. With a secret set, the header `X-Flyght-Signature: sha256=<hex HMAC-SHA256(secret, raw body)>` is added.

## Devices (present only when the module is used)

| Module | Endpoints | WebSocket topic |
|---|---|---|
| `fn::Bno055` | `GET /imu` · `POST /imu/level {clear?}` (operator) · `POST/DELETE /imu/calibration` (admin) · `GET /i2c/scan` | `imu`: `{y, p, r, t, c:[sys,gyro,acc,mag]}` at 15 Hz |
| `fn::LcdI2c` | `GET /lcd` · `POST /lcd {row,text}` / `{clear}` / `{backlight}` / `{preset:"ip"}` (operator) | `lcd`: `{rows[], bl}` |
| `fn::SdCard` | `GET /sd` · `GET /sd/ls?path=` · `GET /sd/file?path=&dl=1` · `POST /sd/file?path=` (raw body) · `DELETE /sd/file?path=` · `POST /sd/mkdir?path=` · `POST /sd/rename {from,to}` · `POST /sd/mount` | — |
| `fn::Tft` | `GET /tft` (size, settings, display list) · `POST /tft/draw {k,a,b,cc,d,c,z,f,s,bg}` (operator) · `POST /tft/settings {rot,bright,inverted,splash}` (admin) | `tft`: `{op}` or `{reload}` |
| `fn::Gps` | `GET /gps` · `GET /gps/track` → `[[lat,lon],...]` · `DELETE /gps/track` (operator) | `gps`: fix, 1 Hz |
| `fn::McuLink` | `GET /mcu?since=` · `POST /mcu/send {line}` (operator) · `PUT /mcu/config {baud,eol}` (admin) | `mcu`: `{lines[], values{}, online}` |

Pages subscribe to a topic by sending `{"t":"sub","id":"imu"}` over `/fn/ws` every few seconds. Modules publish only while someone is subscribed (`{"t":"x","id":"imu","d":{...}}`).

TFT draw ops: `k` = `c` clear, `t` text (`s`, `z` size 1–8, optional `bg`), `r` rectangle (`cc` width, `d` height, `f` filled), `l` line (`cc`,`d` = end point), `o` circle (`cc` radius, `f`). Colors are `#rrggbb`.

## Lights (`fn::Led`, `fn::RgbLed`)

| Method | Path | Role | Notes |
|---|---|---|---|
| GET | `/lights` | viewer | `{lights:[{id, title, rgb, on, color, color2, brightness, effect, period}], effects:[{id, title, colors, period}]}` |
| POST | `/lights/<id>` | operator | `order: "grb"\|"rgb"\|"brg"\|"rbg"\|"gbr"\|"bgr"` (addressable LEDs, admin only, saved at once). Any of `{on: true\|false\|"toggle", color: "#rrggbb", color2, brightness: 0-100, effect: "rainbow", period: ms}`. 422 on a bad colour or effect |

WebSocket topic `lights`: one light's state after each change. MQTT: `<base>/light/<id>/set`
(`ON`, `OFF`, `TOGGLE`, `#rrggbb`, effect id, or the JSON above) and `<base>/light/<id>/state` (retained JSON).

## Visual flows (`fn::Flows`, Pro)

| Method | Path | Role | Notes |
|---|---|---|---|
| GET | `/flows` | viewer | `{allowed, on, rtosAllowed, max, flows:[{id, name, on, rtos, runs, lastRun, active, blocks}]}` |
| GET | `/flows/blocks` | viewer | Block catalogue: `{blocks:[{type, cat, title, icon, desc, inputs, outputs[], params:[{key, label, kind, def, opts}]}]}` |
| GET | `/flows/<id>` | viewer | Full flow (below) |
| PUT | `/flows/new` · `/flows/<id>` | admin | Create / replace. 422 with the reason when invalid, 403 for `rtos` below Enterprise, 409 at 8 flows |
| DELETE | `/flows/<id>` | admin | |
| POST | `/flows/<id>/run` | operator | Fires the flow's Run button trigger(s); body = `{value}` |
| POST | `/flows/hook/<name>` | operator | Fires every HTTP hook trigger with that name; body = `{value}` |
| POST | `/flows/aperture` | operator | An Aperture (AI vision) event: `{"feed":"people","camera":"gate-1","count":7}` or `{"feed":"ppe","camera":"gate-1","violations":2}`. Fires every Aperture trigger with that feed and camera (empty camera = any); the number travels on the wire as `{value}`. 422 when the feed or the number is wrong |

Every flow route except `GET /flows` answers 403 on Free and 409 while the switch is off.

```json
{ "name": "Traffic light", "on": true, "rtos": false,
  "nodes": [ {"id": "n1", "type": "trigger.manual", "x": 30, "y": 120, "p": {}},
             {"id": "n2", "type": "logic.split", "x": 270, "y": 110, "p": {"mode": "serial"}},
             {"id": "n3", "type": "light.set", "x": 520, "y": 0, "p": {"light": "builtin", "power": "on", "color": "#ef4444"}} ],
  "wires": [ ["n1", 0, "n2"], ["n2", 0, "n3"] ] }
```

A wire is `[fromBlock, outputIndex, toBlock]`. WebSocket topic `flows`: `{ev: ["<flowId>:<blockId>", ...]}`
as runs pass through blocks (used for the live highlight).

<!-- flyght-cloud-footer:start -->
---

**Flyght Cloud.** See this device next to your others, from anywhere: [flyght.obayan.id](https://flyght.obayan.id). Sign in, claim the device with its link, and the dashboard you built here is also on the web.
<!-- flyght-cloud-footer:end -->
