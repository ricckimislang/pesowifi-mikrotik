# Piso WiFi Vendo — Step-by-Step Implementation Guide

This guide walks through building, flashing, and deploying a standalone Piso WiFi Vendo system using the files inside `v3/`.

---

## Phase 1 — Create the Captive Portal UI

**Goal**: Build the pages the user sees when they connect to the hotspot.

### 1.1 `v3/portal/index.html` (User Portal)

**What it is**: The captive portal page served by MikroTik when users connect to WiFi.

**How it is used**: MikroTik hotspot redirect sends all HTTP traffic to this file. It uses MikroTik template variables like `$(mac)`, `$(ip)`, `$(identity)`.

**What goes inside**:
- Self-contained HTML + CSS (no external CDN; no internet access yet when captive portal loads)
- Brand header, timer display, coin balance, minutes earned
- "Insert Coin" button to open coin modal
- Modal with countdown timer, Cancel button, animated coin slot
- Session status box (active/inactive, minutes remaining, MAC address)
- Hidden form for MikroTik logout if needed

**Key elements**:
- Hidden inputs: `<input id="hmac" value="$(mac)">`, `<input id="hip" value="$(ip)">`
- Hidden vendor name: `<input id="hname" value="$(identity)">`
- Timer container: `<div id="coinTimer">`
- Coin count display: `<div id="coinCount">`
- Modal overlay: `<div id="coinModal">`

**Step to create**:
1. Create folder `v3/portal/`
2. Write `index.html` with inline `<style>` and inline `<script>` (or reference `portal.js`)
3. All CSS must be self-contained — no external links
4. Use `#fff` and `#4c7a9b` theme colors

### 1.2 `v3/portal/portal.js` (User Portal Logic)

**What it is**: JavaScript that handles the coin flow between the user and the ESP8266.

**How it is used**: Loaded by `index.html`. It communicates with the ESP via AJAX (`10.0.0.2`).

**What goes inside**:
- `ESP_IP` constant: `10.0.0.2`
- `showStatus(msg, type)`: display toast messages
- `openCoin()`: call `POST /api/lock?mac=XX&ip=YY`, open modal, start 60s timer
- `closeCoin()`: call `GET /api/unlock`, close modal, stop timer
- `pollCoinStatus()`: call `GET /api/coinStatus` every 2s, update coin count
- `finalize()`: call `POST /api/finalize`, parse `minutes|mikrotik_ok`, start countdown
- `startCountdown(minutes)`: countdown timer for active session
- `btnDone()`: redirect to internet or show done message

**Step to create**:
1. Create `v3/portal/portal.js`
2. Read `$(mac)` and `$(ip)` from hidden inputs
3. Implement lock/finalize flow with fetch/XHR
4. Handle modal open/close and LED-style UI feedback

### 1.3 `v3/portal/admin.html` (Admin Dashboard)

**What it is**: The admin interface for the machine owner. Accessed by navigating to `10.0.0.2/admin.html`.

**How it is used**: The owner logs in, configures rates, views sales, generates vouchers, updates firmware.

**What goes inside**:
- Login page (before access)
- Tab navigation: Dashboard | System Config | Promo Rates | Vouchers | Firmware | About
- Dashboard: lifetime income, coin total, customer count, uptime, WiFi status, MikroTik status, MAC, IP
- System Config: form with vendo name, WiFi SSID/PW, MikroTik IP/user/pw, coin wait time, admin credentials, pin assignments, IP mode
- Promo Rates: editable table (name, price PHP, minutes, active)
- Vouchers: prefix, amount, quantity, checkbox to add to sales
- Firmware: file upload button, OTA via `/admin/updateMainBin`
- About: firmware version, ESP chip info

**Step to create**:
1. Create `v3/portal/admin.html`
2. Add tab buttons and tab content containers
3. Build each tab with forms and tables
4. Keep all CSS/JS self-contained or reference `admin.js`

### 1.4 `v3/portal/admin.js` (Admin Logic)

**What it is**: JavaScript that sends AJAX to ESP admin API endpoints.

**How it is used**: Loaded by `admin.html`. Every tab calls an ESP endpoint.

**What goes inside**:
- `API_BASE = "http://10.0.0.2/admin/api"`
- `showSpinner()`, `hideSpinner()`: loading overlays
- `showToast(msg, type)`: status messages
- `checkLogin()`: verify `adminSessionValid`
- `loadDashboard()`: `GET /dashboard`, parse pipe-delimited string, update DOM
- `saveSystemConfig()`: read form, `POST /saveSystemConfig?data=...`
- `loadRates()`: `GET /getRates`, parse `ROW_SEP` (`#`) delimited rates, build table
- `saveRates()`: build rate string, `POST /saveRates?data=...`
- `generateVouchers()`: `POST /generateVouchers?pfx=XX&amt=YY&qty=ZZ&sales=0|1`
- `resetStats(type)`: `GET /resetStatistic?type=lifetime|coin|customer`
- `updateFirmware()`: `POST /updateMainBin` with file upload

**Step to create**:
1. Create `v3/portal/admin.js`
2. Implement login gate and session timeout
3. Wire each tab to its corresponding API endpoint
4. Handle pipe-delimited (`|`) and row-delimited (`#`) ESP responses

---

## Phase 2 — MikroTik Router Configuration

**Goal**: Configure MikroTik hAP Lite so users are redirected to the captive portal and the ESP can manage hotspot sessions.

### 2.1 Upload User Portal Files to MikroTik

**What is uploaded**: `index.html` and `portal.js` only.

**Why not admin files?**: The admin dashboard (`admin.html` + `admin.js`) is served directly by the ESP8266 because it communicates exclusively with the ESP API and configures ESP settings. Keeping it on the ESP eliminates CORS issues and keeps admin access separate from the public captive portal.

**How it is used**: MikroTik serves these files from the hotspot `html-directory` when unauthenticated users request HTTP.

**Step to upload**:
1. Open WinBox / WebFig -> Files
2. Create folder `hotspot/` under Files
3. Upload `index.html` and `portal.js` into `hotspot/`
4. **Do not upload** `admin.html` or `admin.js` here — those go to the ESP SPIFFS (see Phase 3.9)
5. Set Hotspot Server Profile -> `html-directory = hotspot`

### 2.2 Import RouterOS Script

**What it is**: `v3/mikrotik/setup.rsc`

**How it is used**: A one-time import script that sets up DHCP, IP pool, hotspot, walled garden, and IP bindings.

**What the script does**:
- Adds IP pool `10.0.0.100-10.0.0.200`
- Adds DHCP server on LAN interface
- Adds hotspot profile with login/logout scripts
- Adds hotspot server on LAN interface
- Walled garden: allows access to `10.0.0.1` (portal) and `10.0.0.2` (ESP)
- IP binding: bypass ESP MAC so it is never redirected
- Creates API user `espapi` with `api,write,policy` policies
- Enables API service

**Step to run**:
1. Open WinBox -> Terminal
2. Run `/import file-name=setup.rsc`
3. Verify DHCP server is running (`/ip/dhcp-server print`)
4. Verify hotspot is running (`/ip/hotspot print`)
5. Verify API service is enabled (`/ip/service print`)

### 2.3 Add ESP IP Binding (Critical)

**Why**: The ESP must NOT be redirected by the hotspot. It needs direct access to the LAN and the MikroTik API.

**Step**:
1. WinBox -> IP -> Hotspot -> IP Bindings
2. Click Add
3. Set MAC address to ESP8266 MAC (printed on serial monitor at boot)
4. Set Type: `bypassed`
5. Set Address: `10.0.0.2` (static)
6. Set To Address: `10.0.0.2`
7. Click OK

### 2.4 Create API User for ESP

**Why**: The ESP needs a MikroTik user to call the RouterOS API for adding hotspot users.

**Step**:
1. WinBox -> System -> Users
2. Add user `espapi`
3. Set password `securepassword123`
4. Set Group: `write` or `full`
5. Enable API service: `/ip/service/enable api`
6. Disable API-SSL if not used: `/ip/service/disable api-ssl`

### 2.5 Hotspot Profile Configuration

**What to set**:
- HTML directory: `hotspot`
- Login page: `index.html`
- Session timeout: match your promo rate max (e.g., 240 minutes)
- Idle timeout: 5 minutes
- Keepalive timeout: 15 minutes
- Cookie lifetime: 1 day (optional)

**Step**:
1. WinBox -> IP -> Hotspot -> Server Profiles
2. Edit default or create new profile
3. Set `html-directory = hotspot`
4. Set `login-by = http-chap, cookie`
5. Set `split-user-domain = no`

---

## Phase 3 — ESP8266 Firmware

**Goal**: Build and flash the Arduino sketch that handles coins, storage, and API.

### 3.1 Arduino IDE Setup

**Step**:
1. Install Arduino IDE (v1.8.x or v2.x)
2. Add ESP8266 board support:
   - File -> Preferences -> Additional Board Manager URLs: `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - Tools -> Board -> Boards Manager -> search "esp8266" -> install
3. Select board: `NodeMCU 1.0 (ESP-12E Module)`
4. Set Flash Size: `4MB (FS:1MB OTA:~1019KB)` — this reserves 1MB for SPIFFS
5. Install libraries (via Library Manager or manual ZIP):
   - `ESP8266WiFi` (included with core)
   - `ESP8266WebServer` (included with core)
   - `ESP8266mDNS` (included with core)
   - `ArduinoOTA` (included with core)
   - `EEPROM` (included with core)
   - Custom `mikrotik_telnet` (local files below)

### 3.2 Create `config.h`

**What it is**: Central constants file for pins, EEPROM addresses, network defaults.

**How it is used**: Included by all other `.cpp` and `.ino` files.

**What goes inside**:
```
FIRMWARE_VERSION = "3.0.0"
DEFAULT_COIN_WAIT_TIME = 60
EEPROM_SIZE = 64
EEPROM_SALES_ADDR = 0
EEPROM_MAGIC = 0xDEAD
WIFI_CONNECT_TIMEOUT = 15000
AP_SSID_PREFIX = "PisoWiFi"
HTTP_PORT = 80
MT_API_PORT = 8728
ADMIN_SESSION_TIMEOUT = 300000
PATH_SYSTEM_CONFIG = "/sys/config.txt"
PATH_RATES = "/sys/rates.txt"
PATH_LOG = "/sys/log.txt"
PATH_VOUCHERS = "/sys/vouchers.txt"
ROW_SEP = '#'
```

**Step**: Create `v3/esp/piso-wifi/config.h`

### 3.3 Create EEPROM Manager

**What it is**: `eeprom_manager.h` + `eeprom_manager.cpp`

**How it is used**: Stores lifetime income, coin total, customer count, and a checksum to detect corruption.

**What goes inside**:
- `struct SalesData { uint32_t lifetimeIncome; uint32_t coinTotal; uint32_t customerCount; uint32_t checksum; }`
- `begin()` — call `EEPROM.begin()`
- `read()` — read struct from EEPROM, validate checksum, return data or zeroed defaults
- `write(data)` — write struct, compute checksum, commit
- `reset(type)` — zero specific fields ("lifetime", "coin", "customer", or "all")

**Step**: Create both files, add to Arduino project tab.

### 3.4 Create SPIFFS Manager

**What it is**: `spiffs_manager.h` + `spiffs_manager.cpp`

**How it is used**: Reads/writes files on ESP flash for system config, promo rates, vouchers, and logs.

**What goes inside**:
- `begin()` — call `SPIFFS.begin()`
- `loadConfig(cfg)` — parse `/sys/config.txt` into `SystemConfig` struct
- `saveConfig(cfg)` — serialize `SystemConfig` to file
- `readFile(path)` — return contents as String
- `writeFile(path, data)` — write String to file
- `appendFile(path, data)` — append String to file
- `log(msg)` — prepend timestamp and append to `/sys/log.txt`

**Step**: Create both files, add to Arduino project tab.

### 3.5 Create MikroTik Telnet Client

**What it is**: `mikrotik_telnet.h` + `mikrotik_telnet.cpp`

**How it is used**: Connects to MikroTik RouterOS via Telnet (port 23) to manage voucher-based hotspot users.

**What goes inside**:
- `WiFiClient client`
- `connect(ip, port, user, pass)` — Telnet connect, wait for prompt, login
- `registerVoucher(voucher, profile)` — send `/ip/hotspot/user add name=VOUCHER limit-uptime=0`
- `addTimeToVoucher(voucher, minutes, validity, dataLimit, comment)` — read current limit-uptime, calculate new, set with comment
- `generateVoucher(voucher, minutes, profile)` — create pre-paid voucher
- CLI command helpers and response parsing

**Step**: Create both files, add to Arduino project tab.

### 3.6 Create Web Server

**What it is**: `web_server.h` + `web_server.cpp`

**How it is used**: Serves all HTTP API endpoints for the portal and admin dashboard.

**What goes inside**:
- `ESP8266WebServer server(80)`
- `ESP8266HTTPUpdateServer updateServer` (for OTA via `/update`)
- Setup routes for portal API (`/api/lock`, `/api/unlock`, `/api/coinStatus`, `/api/finalize`, `/api/rates`)
- Setup routes for admin API (HTTP Basic Auth: `/admin/api/dashboard`, `/admin/api/resetStatistic`, `/admin/api/getSystemConfig`, `/admin/api/saveSystemConfig`, `/admin/api/getRates`, `/admin/api/saveRates`, `/admin/api/generateVouchers`, `/admin/updateMainBin`)
- **Static file routes** for admin portal: `/admin` -> redirect to `/admin.html`, `/admin.html` -> serve from SPIFFS, `/admin.js` -> serve from SPIFFS
- `handleLock()` — set `sessionLocked=true`, enable coin pin, start timer
- `handleFinalize()` — disable coin pin, update EEPROM, call MikroTik Telnet to create voucher, reset state
- `handleDashboard()` — read EEPROM, check WiFi/MikroTik, return pipe-delimited stats
- `handleGenerateVouchers()` — create random codes, save to SPIFFS, optionally add to sales

**Step**: Create both files, add to Arduino project tab.
### 3.7 Create Main Sketch

**What it is**: `piso-wifi.ino`

**How it is used**: 
- Include headers: `config.h`, `eeprom_manager.h`, `spiffs_manager.h`, `mikrotik_telnet.h`, `web_server.h`
- Global objects: `eepromMgr`, `spiffsMgr`, `mtTelnet`, `webServer`
- Global state: `pulseCount`, `coinAmount`, `coinWindowOpen`, `sessionLocked`, `lockedMac`, `lockedIp`
- ISR `onCoinPulse()` — increment `pulseCount`, reset window timer
- `setup()`: begin managers, check EEPROM for config backup (after OTA), load config, set pins, attach interrupt, init WiFi, OTA, web server, mDNS
- `loop()`: feed watchdog, handle clients, update LEDs, check coin timeout, handle backup/restore button
- `initWiFi()` — connect to `sysCfg.wifiSSID` or fallback to AP mode
- `updateLeds()` — blink patterns for ready/insert states
- `handleBackup()` — save EEPROM to SPIFFS file
- `handleRestore()` — load EEPROM from SPIFFS file

**Step**:
1. Create `piso-wifi.ino`
2. Add all `.h` and `.cpp` files as tabs in Arduino IDE (Ctrl+Shift+N for new tab)
3. Verify the sketch compiles
4. Connect ESP8266 NodeMCU via USB
5. Select correct COM port
6. Upload sketch
7. Open Serial Monitor (115200 baud) to verify boot messages

### 3.8 First Boot & Configuration

**Step**:
1. After flashing, ESP boots in AP mode if no WiFi config is saved
2. Connect phone/laptop to AP named `PisoWiFi-xxxx`
3. Open `192.168.4.1` (ESP AP IP)
4. Configure WiFi SSID and password of your MikroTik LAN
5. Set static IP to `10.0.0.2`, gateway `10.0.0.1`
6. Save config — ESP reboots and connects to MikroTik LAN
7. Verify in Serial Monitor: IP should be `10.0.0.2`

### 3.9 Upload Admin Portal Files to ESP SPIFFS

**Why**: The admin dashboard (`admin.html` + `admin.js`) is served by the ESP8266 web server from SPIFFS, not from MikroTik. This keeps admin configuration on the same device that stores the config data.

**Step**:
1. In Arduino IDE, create a folder named `data/` inside your sketch folder (`v3/esp/piso-wifi/`)
2. Copy `admin.html` and `admin.js` into `data/`
3. Install the **ESP8266 Sketch Data Upload** tool if not already installed:
   - Arduino IDE 1.x: Tools -> ESP8266 Sketch Data Upload
   - Arduino IDE 2.x: Use the `esp8266fs` plugin or upload via serial with a separate tool
4. Select the same COM port and board as for sketch upload
5. Click **Tools -> ESP8266 Sketch Data Upload**
6. Wait for upload to complete — this writes `admin.html` and `admin.js` into SPIFFS
7. Verify: open browser to `http://10.0.0.2/admin.html` — the login page should load

**Note**: If you update the admin UI later, repeat this upload step. The system config stored in SPIFFS is preserved as long as you don't reformat the filesystem.

---

## Phase 4 — Wiring & Hardware Assembly

**Goal**: Connect the ESP8266 to the coin acceptor and LEDs.

### 4.1 Pin Mapping (Default)

| ESP Pin | GPIO | Function | Wire Color (Suggested) |
|---------|------|----------|------------------------|
| D6 | GPIO12 | Coin pulse IN | White |
| D7 | GPIO13 | Coin set/enable OUT | Green |
| D3 | GPIO0 | Ready LED | Yellow |
| D4 | GPIO2 | Insert LED | Red |
| D5 | GPIO14 | Insert button (optional) | Blue |
| GND | — | Common ground | Black |
| VIN | — | 5V input (from regulator) | Red |

### 4.2 Coin Acceptor Wiring

**How it works**: Most coin acceptors output a pulse signal per coin. The ESP counts these pulses.

**Step**:
1. Coin acceptor pulse output -> D6 (GPIO12)
2. Coin acceptor enable input -> D7 (GPIO13) — ESP drives this HIGH to enable coin acceptance
3. Common GND between ESP and coin acceptor
4. Coin acceptor needs 12V power — use a separate 12V supply or buck converter from main PSU

### 4.3 LED Indicators

**Step**:
1. Ready LED (slow blink = system ready) -> D3 through 220Ω resistor to GND
2. Insert LED (solid = insert coin now) -> D4 through 220Ω resistor to GND
3. Active-low or active-high configurable via `config.h` (`ledTriggerHigh`)

### 4.4 Power Supply

**Recommended**:
- 12V 5A PSU for coin acceptor
- Buck converter (12V -> 5V) for ESP8266 NodeMCU VIN pin
- Common ground shared between all components

**Step**:
1. Connect PSU 12V to coin acceptor
2. Connect buck converter 12V input to PSU
3. Connect buck converter 5V output to NodeMCU VIN
4. Connect all GNDs together

---

## Phase 5 — Testing & Calibration

**Goal**: Verify the full system works end-to-end before deployment.

### 5.1 Test WiFi & Portal

**Step**:
1. Connect to the vending WiFi SSID
2. Open any HTTP website (e.g., `http://neverssl.com`)
3. You should be redirected to `10.0.0.1/index.html`
4. Verify the portal loads correctly

### 5.2 Test ESP API Directly

**Step**:
1. From a laptop on the same LAN, run:
   ```
   curl -X POST "http://10.0.0.2/api/lock?mac=AA:BB:CC:DD:EE:FF&ip=10.0.0.100"
   ```
2. You should get `OK`
3. Check Serial Monitor — ESP should print "Lock session"
4. Insert a coin (or short D6 to GND briefly to simulate pulse)
5. Run:
   ```
   curl http://10.0.0.2/api/coinStatus
   ```
6. You should get `1|1` (1 pulse, 1 PHP)
7. Run:
   ```
   curl -X POST http://10.0.0.2/api/finalize
   ```
8. You should get `3|1` (3 minutes, MikroTik OK)

### 5.3 Test Admin Dashboard

**Step**:
1. Open `http://10.0.0.2/admin.html`
2. Login with default admin credentials
3. Verify Dashboard shows sales stats
4. Go to System Config tab, modify vendo name, save
5. Verify config persists after ESP reboot

### 5.4 Test MikroTik Session Creation

**Step**:
1. After `finalize`, check WinBox -> IP -> Hotspot -> Users
2. You should see a user with MAC `AA:BB:CC:DD:EE:FF` and uptime limit of 3 minutes
3. Verify the test device can browse the internet
4. Wait 3 minutes — session should expire automatically

### 5.5 Calibrate Coin Acceptor

**Step**:
1. Open `admin.html` -> System Config
2. Set `coinWaitTime` to 60 seconds
3. Insert real coins and verify pulse count matches coin value
4. If your coin acceptor sends multiple pulses per peso, adjust the mapping in `web_server.cpp` `handleFinalize()` or use the promo rates to match pulses to minutes

---

## Phase 6 — Deployment Checklist

**Before leaving the machine running**:

- [ ] User portal files (`index.html`, `portal.js`) uploaded to MikroTik `hotspot/` folder
- [ ] MikroTik script imported and running
- [ ] ESP IP binding (bypassed) added to Hotspot -> IP Bindings
- [ ] Telnet service enabled on MikroTik (default port 23) and user for ESP created
- [ ] ESP8266 flashed with all files (`piso-wifi.ino`, `config.h`, `eeprom_manager.h`, `spiffs_manager.h`, `mikrotik_telnet.h`, `web_server.h`)
- [ ] SPIFFS formatted with 1MB partition
- [ ] Admin files (`admin.html`, `admin.js`) uploaded to ESP SPIFFS via Sketch Data Upload
- [ ] ESP connects to WiFi and gets `10.0.0.2`
- [ ] Coin acceptor wired and enabled pin works
- [ ] LEDs blink correctly (ready = slow, insert = solid)
- [ ] Test coin insertion creates MikroTik user with correct time limit
- [ ] Admin password changed from default
- [ ] Promo rates configured in admin dashboard
- [ ] EEPROM shows zeroed or correct sales data
- [ ] OTA update endpoint tested (optional)
- [ ] Physical enclosure secured and ventilated

---

## Troubleshooting

| Problem | Likely Cause | Fix |
|---------|--------------|-----|
| Portal does not load | MikroTik walled garden missing ESP IP | Add `10.0.0.2` to walled garden |
| ESP not responding | Not on same subnet or IP binding missing | Verify ESP IP binding is "bypassed" |
| Coin not counted | Interrupt not firing | Check D6 wiring, use `INPUT_PULLUP`, verify pulse width |
| MikroTik user not created | API user wrong or API disabled | Check `espapi` credentials, enable API service |
| Sales data lost | EEPROM not committed | `EEPROM.commit()` must be called after write |
| Admin session expires too fast | `ADMIN_SESSION_TIMEOUT` too low | Increase in `config.h` |
| Files not found on ESP | SPIFFS not formatted | Use Arduino IDE Tools -> ESP8266 Sketch Data Upload |

---

## File Inventory

| File | Purpose | Deploy Target | Build Tool |
|------|---------|---------------|------------|
| `v3/portal/index.html` | User captive portal | MikroTik `Files/hotspot/` | Any text editor |
| `v3/portal/portal.js` | Portal logic | MikroTik `Files/hotspot/` | Any text editor |
| `v3/portal/admin.html` | Admin dashboard UI | ESP SPIFFS (`/admin.html`) | Any text editor |
| `v3/portal/admin.js` | Admin AJAX logic | ESP SPIFFS (`/admin.js`) | Any text editor |
| `v3/mikrotik/setup.rsc` | RouterOS config script | MikroTik Terminal | WinBox / WebFig |
| `v3/esp/piso-wifi/piso-wifi.ino` | Main Arduino sketch | ESP flash | Arduino IDE |
| `v3/esp/piso-wifi/config.h` | Constants | ESP flash | Arduino IDE |
| `v3/esp/piso-wifi/eeprom_manager.h/.cpp` | Sales persistence | ESP flash | Arduino IDE |
| `v3/esp/piso-wifi/spiffs_manager.h/.cpp` | File storage | ESP flash | Arduino IDE |
| `v3/esp/piso-wifi/mikrotik_telnet.h/.cpp` | RouterOS Telnet CLI client | ESP flash | Arduino IDE |
| `v3/esp/piso-wifi/web_server.h/.cpp` | HTTP API + file server | ESP flash | Arduino IDE |
| `v3/README.md` | Project overview | — | Any text editor |
| `v3/docs/implementation-guide.md` | This guide | — | Any text editor |

---

*End of guide. Follow each phase in order. Do not skip Phase 2 (MikroTik config) before Phase 3 (ESP firmware) or the ESP will not be able to communicate with the router via Telnet.*
