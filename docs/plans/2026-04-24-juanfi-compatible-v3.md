# JuanFi-Compatible v3 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `v3` work end-to-end like JuanFi: users insert coins on MikroTik captive portal, ESP generates/extends voucher time reliably, browser logs in automatically, and admin config UI saves to ESP safely.

**Architecture:** Keep the **user captive portal** hosted on MikroTik (`v3/portal/*`). Keep the **admin dashboard + config** hosted on the ESP SPIFFS (`v3/portal/admin.*`) so saving config is same-origin to ESP. Portal talks cross-origin to ESP via `http://10.0.0.2` using CORS.

**Tech Stack:** ESP8266 (Arduino core), SPIFFS, EEPROM, ESP8266WebServer, MikroTik RouterOS via Telnet CLI, MikroTik hotspot HTML variables (`$(...)`), plain HTML/JS.

---

## File Map (what changes and why)

**ESP firmware**
- Modify: `v3/esp/piso-wifi/spiffs_manager.h` — extend `SystemConfig` to include fields used by the code + UI.
- Modify: `v3/esp/piso-wifi/spiffs_manager.cpp` — include `config.h`, zero-init before parsing, parse the same pipe-delimited order the UI expects, and expose getters for fields used elsewhere.
- Modify: `v3/esp/piso-wifi/web_server.cpp` — (1) require auth for HTTP OTA, (2) return voucher string from `/api/finalize`, (3) allow `Authorization` header in CORS for admin if needed.
- Modify: `v3/esp/piso-wifi/piso-wifi.ino` — gate ArduinoOTA behind config + set password.
- Modify: `v3/esp/piso-wifi/mikrotik_telnet.cpp` — make voucher add/extend robust (find by name, correct RouterOS vars, read + validate responses).

**MikroTik portal + template**
- Modify: `v3/portal/index.html` — admin link points to ESP, add hidden login form + needed hotspot variables for auto-login.
- Modify: `v3/portal/portal.js` — after finalize, receive voucher and submit login automatically; show voucher for manual fallback.
- Modify: `v3/mikrotik/setup.rsc` — enable `http-pap` (to avoid CHAP MD5 complexity) and ensure walled-garden allows ESP admin host.

**Docs**
- Modify: `v3/README.md` — clarify “admin UI is on ESP” and the exact URLs.

---

### Task 1: Fix admin access path (save config reliably)

**Files:**
- Modify: `v3/portal/index.html`
- Modify (optional): `v3/mikrotik/setup.rsc`
- Docs: `v3/README.md`

- [ ] **Step 1: Change admin link to ESP host**

Update the link so it never points to MikroTik filesystem:

```html
<!-- old: <a href="admin.html">Admin Dashboard</a> -->
<a href="http://10.0.0.2/admin.html" target="_blank" rel="noopener">Admin Dashboard</a>
```

Why: `index.html` is served by MikroTik, so `admin.html` relative path resolves to MikroTik. Point it to the ESP explicitly so admin JS stays same-origin to ESP and can save config via `/admin/api/*`.

- [ ] **Step 2: Ensure MikroTik walled-garden allows ESP host**

In `v3/mikrotik/setup.rsc`, ensure at least one rule allows `10.0.0.2` before login:

```rsc
/ip hotspot walled-garden add dst-address=10.0.0.2 action=allow comment="ESP Admin/API"
```

- [ ] **Step 3: Document the intended admin flow**

In `v3/README.md`, state:
- User portal: `http://10.0.0.1/` (MikroTik)
- Admin UI: `http://10.0.0.2/admin.html` (ESP)

---

### Task 2: Make `SystemConfig` consistent (compile + correct runtime behavior)

**Files:**
- Modify: `v3/esp/piso-wifi/spiffs_manager.h`
- Modify: `v3/esp/piso-wifi/spiffs_manager.cpp`
- Modify: `v3/esp/piso-wifi/web_server.cpp` (small references)

- [ ] **Step 1: Extend `SystemConfig` to match all fields used**

In `v3/esp/piso-wifi/spiffs_manager.h`, add the missing fields that are used by:
- `web_server.cpp` (`checkInternet`, `voucherProfile`, etc.)
- `admin.js` field order
- `handleGetSystemConfig()`/`saveConfig()`

Use a struct like:

```cpp
struct SystemConfig {
  char vendoName[32];
  char wifiSSID[32];
  char wifiPW[64];
  char mtIp[16];
  char mtUser[32];
  char mtPw[64];
  uint16_t coinWaitTime;

  char adminUser[32];
  char adminPw[64];

  uint8_t abuseCount;
  uint8_t banMinutes;

  uint8_t pinCoinSlot;
  uint8_t pinCoinSet;
  uint8_t pinReadyLed;
  uint8_t pinInsertLed;
  uint8_t lcdScreen;      // keep placeholder for JuanFi-style ordering
  uint8_t pinInsertBtn;

  uint8_t checkInternet;
  char voucherPrefix[4];  // e.g. "PW"
  char welcomeMsg[64];
  uint8_t setupDone;
  uint8_t voucherLoginOpt;
  char voucherProfile[16]; // e.g. "default"
  uint16_t voucherValidity;

  uint8_t ledTriggerHigh;

  uint8_t ipMode;        // 0=DHCP, 1=Static
  char localIp[16];
  char gatewayIp[16];
  char subnetMask[16];
  char dnsServer[16];
};
```

- [ ] **Step 2: Include `config.h` where macros are used**

In `v3/esp/piso-wifi/spiffs_manager.cpp`, add:

```cpp
#include "config.h"
```

so `PATH_SYSTEM_CONFIG`, `DEFAULT_*`, etc. are defined.

- [ ] **Step 3: Zero-init before parsing to avoid garbage**

At the start of the “data exists” parsing path:

```cpp
memset(&cfg, 0, sizeof(cfg));
```

- [ ] **Step 4: Parse pipe-delimited config in the same order the UI expects**

Replace the current “skip N fields” logic with explicit reads in the exact order used by `admin.js`:

```cpp
// helper: safely copy token or default
auto nextStr = [&](char* dest, size_t len) {
  if (token) { strlcpy(dest, token, len); token = strtok(NULL, "|"); }
  else { dest[0] = '\0'; }
};
auto nextU8 = [&]() -> uint8_t { uint8_t v = token ? (uint8_t)atoi(token) : 0; if (token) token = strtok(NULL, "|"); return v; };
auto nextU16 = [&]() -> uint16_t { uint16_t v = token ? (uint16_t)atoi(token) : 0; if (token) token = strtok(NULL, "|"); return v; };

nextStr(cfg.vendoName, sizeof(cfg.vendoName));
nextStr(cfg.wifiSSID, sizeof(cfg.wifiSSID));
nextStr(cfg.wifiPW, sizeof(cfg.wifiPW));
nextStr(cfg.mtIp, sizeof(cfg.mtIp));
nextStr(cfg.mtUser, sizeof(cfg.mtUser));
nextStr(cfg.mtPw, sizeof(cfg.mtPw));
cfg.coinWaitTime = nextU16();

nextStr(cfg.adminUser, sizeof(cfg.adminUser));
nextStr(cfg.adminPw, sizeof(cfg.adminPw));

cfg.abuseCount = nextU8();
cfg.banMinutes = nextU8();

cfg.pinCoinSlot = nextU8();
cfg.pinCoinSet  = nextU8();
cfg.pinReadyLed = nextU8();
cfg.pinInsertLed = nextU8();
cfg.lcdScreen = nextU8();
cfg.pinInsertBtn = nextU8();

cfg.checkInternet = nextU8();
nextStr(cfg.voucherPrefix, sizeof(cfg.voucherPrefix));
nextStr(cfg.welcomeMsg, sizeof(cfg.welcomeMsg));
cfg.setupDone = nextU8();
cfg.voucherLoginOpt = nextU8();
nextStr(cfg.voucherProfile, sizeof(cfg.voucherProfile));
cfg.voucherValidity = nextU16();
cfg.ledTriggerHigh = nextU8();

cfg.ipMode = nextU8();
nextStr(cfg.localIp, sizeof(cfg.localIp));
nextStr(cfg.gatewayIp, sizeof(cfg.gatewayIp));
nextStr(cfg.subnetMask, sizeof(cfg.subnetMask));
nextStr(cfg.dnsServer, sizeof(cfg.dnsServer));
```

- [ ] **Step 5: Make `saveConfig()` write the same order**

Build the pipe-delimited string in the same field order (matching the parser).

- [ ] **Step 6: Quick compile check**

Arduino IDE / CLI build of `v3/esp/piso-wifi/piso-wifi.ino` should compile with no missing fields.

---

### Task 3: Lock down OTA (match JuanFi expectation: not wide-open)

**Files:**
- Modify: `v3/esp/piso-wifi/piso-wifi.ino`
- Modify: `v3/esp/piso-wifi/web_server.cpp`

- [ ] **Step 1: Require HTTP auth for `/update`**

Change:

```cpp
updateServer.setup(&server);
```

to:

```cpp
updateServer.setup(&server, "/update", sysCfg.adminUser, sysCfg.adminPw);
```

- [ ] **Step 2: Add config flag to enable ArduinoOTA**

Add a config field (recommended): `uint8_t otaEnabled;` and expose it in the config string/UI (if you don’t want UI changes, default it to 0 and allow enabling only by editing config file).

Then in `setup()`:

```cpp
if (sysCfg.otaEnabled) {
  ArduinoOTA.setHostname("piso-wifi");
  ArduinoOTA.setPassword(sysCfg.adminPw);
  ArduinoOTA.begin();
}
```

And in `loop()`:

```cpp
if (sysCfg.otaEnabled) ArduinoOTA.handle();
```

- [ ] **Step 3: Verify OTA behavior**

Expected:
- Hitting `http://10.0.0.2/update` prompts for Basic Auth.
- ArduinoOTA is off unless enabled.

---

### Task 4: Fix MikroTik Telnet voucher commands (reliable create + extend)

**Files:**
- Modify: `v3/esp/piso-wifi/mikrotik_telnet.cpp`

- [ ] **Step 1: Make `registerVoucher()` validate success**

After sending the command, read response and return `false` if it contains typical RouterOS error patterns:

```cpp
bool ok = sendCommand(cmd.c_str());
String resp = readResponse();
if (!ok) return false;
if (resp.indexOf("failure") >= 0 || resp.indexOf("error") >= 0 || resp.indexOf("already have") >= 0) return false;
return true;
```

- [ ] **Step 2: Rewrite `addTimeToVoucher()` using `find name=...`**

Use one RouterOS line that:
1) finds user by name
2) reads current limit-uptime
3) sets new limit-uptime to current + minutes
4) updates comment

Example (single-line script):

```cpp
String cmd =
  ":local id [/ip hotspot user find name=\"" + String(voucher) + "\"];"
  ":if ([:len $id]=0) do={:put \"ERR no user\"} else={"
    ":local lpt [/ip hotspot user get $id limit-uptime];"
    ":local nlu ($lpt + " + String(minutes) + "m);"
    "/ip hotspot user set $id limit-uptime=$nlu comment=\"" + String(validity) + "m," + String(minutes) + ",1," + (comment?String(comment):"") + "\";"
  "}";
```

Send + read response, return false on `ERR`/`failure`/`error`.

- [ ] **Step 3: Fix data-limit script (if used)**

If you want to support `dataLimit`, use correct `$dtl` variable usage:

```cpp
String dataCmd =
  ":local id [/ip hotspot user find name=\"" + String(voucher) + "\"];"
  ":if ([:len $id]=0) do={:put \"ERR no user\"} else={"
    ":local dtl [/ip hotspot user get $id limit-bytes-total];"
    ":local add (" + String(dataLimit) + "*1048576);"
    ":local tdtl ($dtl + $add);"
    ":if ($dtl=0) do={:set tdtl $add};"
    "/ip hotspot user set $id limit-bytes-total=$tdtl;"
  "}";
```

- [ ] **Step 4: Make telnet reads deterministic**

In `readUntilPrompt()` / `readResponse()`, increase timeout to `MT_TELNET_TIMEOUT` and always consume the whole prompt before returning to reduce buffer buildup.

---

### Task 5: Fix voucher generation (space + collision) and return voucher to portal

**Files:**
- Modify: `v3/esp/piso-wifi/web_server.cpp`
- Modify: `v3/esp/piso-wifi/mikrotik_telnet.cpp` (if needed for error detection)

- [ ] **Step 1: Expand voucher space**

Change:

```cpp
String voucher = String(sysCfg.voucherPrefix) + String(random(1000, 9999));
```

to something like:

```cpp
String voucher = String(sysCfg.voucherPrefix) + String(random(100000, 999999));
```

- [ ] **Step 2: Add retry loop for collision**

In `/api/finalize`, retry N times:

```cpp
String voucher = "";
for (int attempt = 0; attempt < 10; attempt++) {
  String candidate = String(sysCfg.voucherPrefix) + String(random(100000, 999999));
  if (mtTelnet.registerVoucher(candidate.c_str(), sysCfg.voucherProfile) &&
      mtTelnet.addTimeToVoucher(candidate.c_str(), minutes, minutes, 0, sysCfg.vendoName)) {
    voucher = candidate;
    mtOk = true;
    break;
  }
}
```

- [ ] **Step 3: Return voucher string to client**

Change finalize response from:

```cpp
String resp = String(minutes) + "|" + (mtOk ? "1" : "0");
```

to:

```cpp
String resp = String(minutes) + "|" + (mtOk ? "1" : "0") + "|" + (mtOk ? voucher : "");
```

---

### Task 6: Portal auto-login to MikroTik after voucher is generated

**Files:**
- Modify: `v3/portal/index.html`
- Modify: `v3/portal/portal.js`
- Modify: `v3/mikrotik/setup.rsc`

- [ ] **Step 1: Enable `http-pap` in hotspot profile**

In `v3/mikrotik/setup.rsc`, set login-by to include `http-pap` so the portal can POST username/password without CHAP hashing:

```rsc
/ip hotspot profile set [ find default=yes ] login-by=http-chap,http-pap,mac-cookie
```

- [ ] **Step 2: Add hidden login form + MikroTik variables**

In `v3/portal/index.html`, include:

```html
<form id="mtLoginForm" method="post" action="$(link-login-only)" style="display:none">
  <input type="hidden" name="username" id="mtUser">
  <input type="hidden" name="password" id="mtPass">
  <input type="hidden" name="dst" value="$(link-orig)">
  <input type="hidden" name="popup" value="true">
</form>
```

And expose the login URL if you prefer `data-*`:

```html
<body data-login-url="$(link-login-only)" data-link-orig="$(link-orig)">
```

- [ ] **Step 3: Submit login form after finalize**

In `v3/portal/portal.js`, after parsing finalize response:

```js
const ok = parts[1] === "1";
const voucher = parts[2] || "";
if (ok && voucher) {
  // show voucher for manual fallback
  showToast(`Voucher: ${voucher}`);
  const f = document.getElementById("mtLoginForm");
  document.getElementById("mtUser").value = voucher;
  document.getElementById("mtPass").value = ""; // no password
  f.submit();
  return;
}
```

If you decide vouchers should have a password, align both:
- create user with password in RouterOS
- submit that same password here

- [ ] **Step 4: Handle failure path without banning legit users**

If finalize fails due to MikroTik unreachable, show a user-friendly message and avoid triggering “MAC abuse” bans based only on connectivity failure (optional improvement).

---

### Task 7: Verification checklist (manual, hardware-in-the-loop)

**Files:**
- None (runbook only)

- [ ] **Step 1: Flash ESP**

Build and upload `v3/esp/piso-wifi/piso-wifi.ino`, then upload SPIFFS data including `admin.html` and `admin.js`.

- [ ] **Step 2: MikroTik setup**

Run `v3/mikrotik/setup.rsc`, upload `v3/portal/index.html` + `v3/portal/portal.js` into `Files/hotspot/`.

- [ ] **Step 3: Admin config save test**

Open `http://10.0.0.2/admin.html` → login → change a field → Save → reboot ESP → confirm persisted.

- [ ] **Step 4: Coin flow test**

On a client:
1) Connect to hotspot → see portal
2) Click Insert Coin → coin window opens
3) Insert coin pulses → finalize
4) Portal should auto-submit login
5) Internet should work until voucher uptime ends

- [ ] **Step 5: Negative tests**

Unplug MikroTik:
- finalize should return `mtOk=0` and portal should not “connect”.
- no permanent bans should occur for normal retries.

---

## Self-Review (plan vs findings)

- Finding 5 (admin link): fixed in Task 1.
- Finding 3 (config mismatch): fixed in Task 2 (struct + include + parser + save).
- Findings 1–2 (OTA): fixed in Task 3.
- Finding 6 (telnet script): fixed in Task 4.
- Finding 8 (voucher space/collision): fixed in Task 5.
- Finding 4 (voucher flow/login): fixed in Task 6.

---

## Execution Handoff

Plan saved to `v3/docs/plans/2026-04-24-juanfi-compatible-v3.md`.

Two execution options:
1) Subagent-Driven (recommended) — dispatch one task at a time + review between tasks
2) Inline Execution — implement tasks in this session with checkpoints

Which approach?

