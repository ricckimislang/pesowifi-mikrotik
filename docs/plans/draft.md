# Materials
- ESP8266
- MikroTik hAP Lite
- Coin Acceptor
- 12V Power Supply
- Modem/AP Router (provides WiFi for users)

---

# Network Topology
- **ISP** → **MikroTik hAP Lite** → **Modem/AP Router** (WiFi for users)
- **Captive Portal IP**: `10.0.0.1`
- **ESP8266** connects to MikroTik wirelessly via **MikroTik API**
- **Admin Dashboard** accessible at `/admin` on the captive portal

---

# System Architecture

## Roles and Responsibilities

### MikroTik Router (Session Manager Only)
- Host the captive portal (hotspot)
- Redirect users to `10.0.0.1` captive portal
- Manage internet sessions: create user, extend time, apply bandwidth limits
- Handle user authentication and session timeouts
- **Does NOT store income or sales records**

### ESP8266 (Hardware Controller + Sales Database)
- Handle physical coin insertion via pulse signals from the coin acceptor
- Lock the session to a single user's MAC address upon "Insert Coin" request
- **Single-user only** — no queue handling
- Enable coin detection after session lock
- Count pulses and convert coin amount into time using promo rates
- Send API requests to MikroTik to create or extend hotspot sessions
- **60-second timeout** for coin insertion window; resets if a coin is inserted

### Storage Architecture

#### EEPROM (Flash Memory) — Sales Data
Updated every time coins are inserted:
- Lifetime income
- Coin totals
- Customer count

#### SPIFFS (File Storage) — System Files
- System settings
- Promo rates
- Configuration files
- Logs

### Captive Portal Interface
- Serves at `10.0.0.1`
- **User Portal**: Check time / Insert Coin button
- **Admin Dashboard** (`/admin`) for configuring:
  - Promo rates
  - Vendo name
  - WiFi SSID/password
  - Router settings
- Uploaded into the MikroTik hotspot folder (`hotspot/`)
- Files: HTML, JavaScript, `config.js`

---

# Verified System Flow (Realistic Deployment Flow)

1. **ISP Internet** feeds into the **MikroTik Router**.
2. MikroTik connects to **Modem/AP Router** broadcasting WiFi.
3. **User connects to WiFi**.
4. **MikroTik redirects to Captive Portal** at `10.0.0.1`.
5. **User clicks "Insert Coin"** on the portal page.
6. **Portal sends request to ESP** to lock the user's **MAC address**.
7. **ESP enables coin acceptor** and starts a **60-second timer**.
8. **User inserts coin**.
9. **ESP counts pulses** and **calculates time** based on configured promo rates.
10. **ESP updates EEPROM** (lifetime income, coin totals, customer count).
11. **ESP sends API request to MikroTik** to create or extend the hotspot session.
12. **MikroTik creates or extends hotspot session** for the user.
13. **Login script runs**.
14. **User gets internet access**.
15. **Session timer expires**.
16. **MikroTik disconnects user**.

---

# MikroTik Configuration Requirements

## IP Bindings Exception
Add the ESP's MAC address and IP to **Hotspot → IP Bindings** with **Type: bypassed** so unauthenticated users can reach the ESP and the captive portal at `10.0.0.1`.

---

# Final Validation of Your Plan

Your design is **technically correct and deployable** if these conditions are met:

- [ ] MikroTik hotspot is properly configured.
- [ ] ESP has static IP and is added to **IP Bindings (bypassed)**.
- [ ] Portal files (HTML/JS/`config.js`) are uploaded correctly into the MikroTik hotspot folder.
- [ ] MikroTik API user is created for ESP communication.
- [ ] Login/logout scripts are applied on the MikroTik router.
- [ ] Coin pulse wiring is stable between ESP and coin acceptor.
- [ ] MAC locking logic (single-user, 60s timeout) is implemented in the ESP firmware.
- [ ] EEPROM write logic for lifetime income, coin totals, and customer count is implemented.
- [ ] SPIFFS stores system settings, promo rates, and configuration files.