# Architecture

Block diagrams of how FlyghtNinno works inside the ESP32. The diagrams are Mermaid (GitHub and
most Markdown viewers render them).

**Contents** — [Layers](#1-layers) · [Modules and dependencies](#2-modules-and-dependencies) ·
[Boot sequence](#3-boot-sequence) · [Tasks and threads](#4-tasks-and-threads) ·
[HTTP request path](#5-http-request-path) · [First connection: captive portal](#6-first-connection-captive-portal) ·
[Sessions and roles](#7-sessions-and-roles) · [A widget change](#8-a-widget-change) ·
[Visual flows engine](#9-visual-flows-engine) · [Lights](#10-lights) · [Storage](#11-storage-nvs) ·
[Editions](#12-editions-and-feature-switches) · [Release packaging](#13-release-packaging) · [Limits](#14-limits)

---

## 1. Layers

```mermaid
flowchart TB
    subgraph L4["Your sketch"]
        S["setup(): dev.page(...), dev.use(...), dev.begin()<br/>loop(): dev.loop(), dev.ui().set(...)"]
    end
    subgraph L3["Features (fn::Module)"]
        direction LR
        F1["Ui"] --- F2["Auth"] --- F3["Mqtt"] --- F4["Webhook"] --- F5["System"]
        F6["Wifi"] --- F7["Ap"] --- F8["Captive"] --- F9["Mdns"]
        F10["Light"] --- F11["Flows"] --- F12["Lcd · Tft · Sd · Gps · Imu · Mcu"]
    end
    subgraph L2["Core"]
        direction LR
        C1["App<br/>container"] --- C2["Bus<br/>events"] --- C3["Scheduler"] --- C4["Features"]
        C5["Web<br/>HTTP + WS"] --- C6["Json"] --- C7["Kv / NVS"] --- C8["Log"]
    end
    subgraph L1["ESP32 Arduino core 3.x / ESP-IDF 5"]
        direction LR
        I1["esp_http_server"] --- I2["esp-mqtt"] --- I3["WiFi / lwIP"] --- I4["NVS"] --- I5["mbedTLS"] --- I6["FreeRTOS"]
    end
    L4 --> L3 --> L2 --> L1
```

No external Arduino libraries: everything below the core layer ships with the ESP32 core.

## 2. Modules and dependencies

```mermaid
flowchart LR
    web[web] --> auth[auth]
    web --> sys[sys]
    web --> captive[captive]
    ap[ap] --> captive
    ap -.-> wifi[wifi]
    web --> ui[ui]
    auth -.-> ui
    ui --> mqtt[mqtt]
    ui --> webhook[webhook]
    web --> flows[flows]
    ui -.-> flows
    mqtt -.-> flows
    web -.-> light[light / led / rgb]
    mqtt -.-> light
    web -.-> devices[lcd · tft · sd · gps · imu · mcu]
    mdns[mdns]
```

Solid arrow = required, dotted = optional (`?` in `deps()`). `App::begin()` sorts modules
topologically, so `dev.use(...)` order never matters. A module whose required dependency failed is
skipped; the rest keep running.

## 3. Boot sequence

```mermaid
sequenceDiagram
    autonumber
    participant Sk as Sketch setup()
    participant App as fn::App
    participant M as Modules
    participant Bus as Event bus
    Sk->>App: dev.page(...), dev.use(...), builders
    Sk->>App: dev.begin()
    App->>App: log banner + license notice
    App->>App: sort modules by deps()
    loop every module, in order
        App->>M: begin(app)
        M-->>App: true / false (failed modules are marked, others continue)
    end
    App->>Bus: emit AppReady
    Bus->>M: onEvent(AppReady) - Flows loads saved flows, fires "On start-up"
    Sk->>App: loop(): dev.loop() forever
```

## 4. Tasks and threads

```mermaid
flowchart TB
    subgraph LoopTask["Arduino loop task (core 1)"]
        L1["App::loop()"] --> L2["posted jobs (app.post)"]
        L2 --> L3["Scheduler timers"]
        L3 --> L4["each Module::loop()"]
        L4 --> L5["Flows on the main loop"]
    end
    subgraph Http["httpd task"]
        H1["route handler"] -->|"validate, App::Guard"| H2["reply"]
        H1 -->|"app.post(...) for changes"| L2
    end
    subgraph Mqtt["esp-mqtt task"]
        M1["message"] -->|app.post| L2
    end
    subgraph Rtos["fn_flow task (one per RTOS flow, Enterprise)"]
        R1["flow logic: wait / if / repeat / split"] -->|"device & action blocks via app.post"| L2
    end
    subgraph Workers["worker tasks"]
        W1["fn_flowhttp: HTTP request block"] -->|"resume() via app.post"| L2
        W2["fn_webhook: webhook delivery"]
    end
```

Rule of thumb: **state changes always happen on the loop task**. Other tasks validate, then post
the work there. `App::Guard` protects data read by the HTTP task (widget values, lists).

## 5. HTTP request path

```mermaid
sequenceDiagram
    participant B as Browser / client
    participant H as esp_http_server
    participant W as fn::Web dispatch
    participant A as Auth
    participant R as Route handler
    participant L as Loop task
    B->>H: GET/POST /fn/api/v1/...
    H->>W: matched route (wildcard *)
    W->>A: authenticate (Bearer, fn_s cookie or ?t=)
    A-->>W: role + user (none, viewer, operator, admin)
    alt role below route minimum
        W-->>B: 401 / 403 JSON error
    else allowed
        W->>R: handler(Request, Response)
        R->>R: parse + validate input
        R-->>B: JSON reply (200 / 4xx)
        R->>L: app.post(apply change)
    end
```

## 6. First connection: captive portal

```mermaid
sequenceDiagram
    participant P as Phone
    participant AP as Ap (SoftAP + DHCP)
    participant D as Captive (DNS catch-all)
    participant W as Web
    P->>AP: join "name-XXXX" (DHCP option 114 = portal URL)
    P->>D: DNS connectivitycheck.gstatic.com?
    D-->>P: 192.168.4.1
    P->>W: GET /generate_204
    W-->>P: 302 → http://192.168.4.1/
    Note over P: the OS opens the portal window
    P->>W: setup wizard / sign in
    W->>D: accept(phone IP)
    P->>W: GET /generate_204
    W-->>P: 204 "internet OK" - the phone stays connected
```

## 7. Sessions and roles

```mermaid
flowchart LR
    LOGIN["POST /auth/login<br/>(password, username?)"] --> CHECK{"owner / account / guest?"}
    CHECK -->|owner| T0["token uid=0, role admin"]
    CHECK -->|account (rbac on)| T1["token uid=1..254, role from account"]
    CHECK -->|guest (guest on)| T2["token uid=255, guest role"]
    T0 & T1 & T2 --> SIGN["HMAC-SHA256(role.uid.sid.exp.epoch)<br/>key = per-device secret in NVS"]
    SIGN --> COOKIE["cookie fn_s / Bearer"]
    COOKIE --> REQ["each request: verify signature, expiry, epoch"]
    REQ --> ROLE["role: owner = admin; account = its CURRENT role;<br/>guest = current guest role; switched off = signed out"]
```

Sign out everywhere increases the epoch, which invalidates every token at once.

## 8. A widget change

```mermaid
flowchart LR
    SRC1["Browser"] -->|POST /state or WS| APPLY
    SRC2["MQTT set/<id>"] --> APPLY
    SRC3["HTTP API"] --> APPLY
    SRC4["Flow: Set widget"] --> APPLY
    SRC5["Sketch: ui.set()"] --> STORE
    APPLY["Ui::apply (loop task)<br/>parse + validate"] --> CB["your callback<br/>void(bool) / void(float) ..."]
    APPLY --> STORE["widget value"]
    STORE --> PUSH["WebSocket push (every 150 ms)"]
    STORE --> PUB["MQTT state/<id>"]
    STORE --> HOOK["Webhook (on change)"]
    STORE --> LIS["listeners: Flows 'Widget changes' trigger"]
```

## 9. Visual flows engine

A flow is a graph of blocks. A **run** is a token that walks the graph.

```mermaid
flowchart LR
    TR(["Trigger fires"]) --> TOK["new token<br/>(value, stack)"]
    TOK --> EX["execute block"]
    EX -->|"next(out)"| FOL{"wires from out"}
    FOL -->|none| END["branch ends → pop stack<br/>(serial / repeat continuation)"]
    FOL -->|one| EX
    FOL -->|"several (parallel)"| CL["clone token per extra wire"] --> EX
    EX -->|"wait(ms)"| WAIT["token sleeps, loop keeps running"] --> FOL
    EX -->|async| ASY["worker task → resume()"] --> FOL
```

| Situation | What the engine does |
|---|---|
| One output, several wires | **Parallel**: one token per wire, all advance together |
| Split block, mode *serial* | Outputs 1, 2, 3 run one after another; the continuation is pushed on the token's stack |
| Repeat N | Runs `each` N times (the block re-enters itself through the stack), then `done` |
| Wait | Non-blocking: the token stores a wake-up time |
| HTTP request | Runs on `fn_flowhttp`, the token resumes when the reply arrives |
| Step budget | At most 48 block steps per loop pass, so a loop without a Wait cannot freeze the device |
| RTOS flow | Logic runs on its own task; device and action blocks are posted to the loop task |
| Pins | Saving fails if a GPIO block uses a pin owned by a module, the flash or USB |

## 10. Lights

```mermaid
flowchart LR
    IN["color() · brightness() · effect()<br/>HTTP · MQTT · flows · Lights page"] --> ST["State<br/>on, color, color2, brightness, fx, period"]
    ST --> FX["fxColor(fx, a, b, period, t)<br/>pure function of time"]
    FX --> BR["× brightness"] --> GM["gamma"] --> OUT{"output"}
    OUT -->|"Led"| PWM["LEDC PWM / digitalWrite"]
    OUT -->|"RgbLed pixel"| RMT["rgbLedWrite (RMT)"]
    OUT -->|"RgbLed 3 pins"| PWM3["3 × LEDC"]
    ST -->|"2 s after the last change"| NVS[("NVS fn.light")]
    ST --> WS["WS topic 'lights' + MQTT state"]
```

The loop renders 50 frames per second and only writes the LED when the colour changed. The web
previews run the same `fxColor` maths in JavaScript, so they match the LED exactly.

## 11. Storage (NVS)

| Namespace | Owner | Contents |
|---|---|---|
| `fn.sys` | App | display name, hostname |
| `fn.auth` | Auth | password hash + salt, session secret, epoch, guest role, session days, API tokens, accounts |
| `fn.feat` | Features | switches changed in Settings |
| `fn.wifi` | Wifi | up to 8 networks with priority |
| `fn.ap` | Ap | SSID, password, mode, channel |
| `fn.ui` | Ui | dashboard layout (order, sizes, hidden) |
| `fn.mqtt` | Mqtt | broker, credentials, base topic, options |
| `fn.hook` | Webhook | URL, secret, interval |
| `fn.light` | Light | last state of each light |
| `fn.flows` | Flows | index + one blob per flow |
| `fn.imu` · `fn.tft` · `fn.mcu` | devices | calibration and settings |

*Factory reset* erases every `fn.*` namespace. Values in code are only first-boot defaults.

## 12. Editions and feature switches

The edition comes from the license key only. `App::begin()` verifies the key (ECDSA P-256 /
SHA-256 with mbedTLS, against the public keys compiled into the archive) before any module
starts. No key or a rejected key means evaluation mode: every edition for 30 minutes, then
`App::loop()` stops running modules and the web server answers 402 to everything but
`/auth/state`, until the next reset.

```mermaid
flowchart LR
    KEY["License key<br/>dev.license(...)"] -->|"ECDSA P-256 check"| ED
    ED["Edition<br/>Free · Pro · Enterprise"] --> AL{"allowed?<br/>edition ≥ feature minimum"}
    DEF["defaultOn() in code"] --> SW{"switched on?<br/>NVS value, else default"}
    UI["Settings → Plan & features"] --> SW
    AL -->|yes| AND(("AND"))
    SW -->|yes| AND
    AND --> ON["features().on(id) = true"]
```

`on()` is cached in RAM, so modules can check it on every request and loop pass.

## 13. Release packaging

The library is delivered as public headers plus one precompiled archive per chip
(`src/<mcu>/libFlyghtEdge.a` for ESP32, S2, S3, C3, C5 and C6), together with the examples and
the documentation (`library.properties` has `precompiled=full`). Each archive is tied to the
ESP32 core major.minor it was built for, which is checked when your sketch compiles.

## 14. Limits

| Item | Limit |
|---|---|
| Saved Wi-Fi networks | 8 |
| API tokens | 8 |
| User accounts | 8 |
| HTTP routes + assets | 128 handlers |
| HTTP sockets | 8 (oldest idle one is recycled) |
| Flows | 8 flows × 40 blocks × 80 wires, 16 runs in flight per flow |
| Flow interval trigger | ≥ 50 ms |
| Text widget | 128 characters |
| Log ring buffer | last lines kept in RAM (System page) |
