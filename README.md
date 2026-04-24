# Piso WiFi Vendo v3

Standalone coin-operated WiFi hotspot system using ESP8266, MikroTik hAP Lite, and a coin acceptor.

## Architecture

| Component | Role |
|-----------|------|
| **MikroTik hAP Lite** | Hotspot gateway, session manager, bandwidth control |
| **ESP8266 NodeMCU** | Hardware controller, coin detection, sales database, API server |
| **Coin Acceptor** | Physical coin insertion, pulse output |
| **Modem/AP Router** | WiFi access point for users |

## Storage

- **EEPROM** (flash): Lifetime income, coin totals, customer count
- **SPIFFS** (file system): System settings, promo rates, vouchers, logs
- **MikroTik**: Session data only (no sales records)

## Features
- Coin-operated WiFi sessions (voucher-based via MikroTik)
- MikroTik RouterOS Telnet CLI integration
- Admin dashboard (HTTP Basic Auth)
- Promo rates configuration
- EEPROM sales persistence
- OTA firmware updates with config backup
- Coin slot interrupt handling
- MAC abuse/ban protection
- Internet connectivity check

## Network Topology

```
ISP -> MikroTik hAP Lite -> Modem/AP Router -> Users
              |
              -> ESP8266 (static IP 10.0.0.2)
```

- User Captive Portal: `10.0.0.1` (hosted on MikroTik hotspot folder)
- ESP8266 Admin Portal: `10.0.0.2/admin.html` (served from ESP SPIFFS)
- ESP API: `10.0.0.2`

## Project Structure

```
v3/
├── portal/
│   ├── index.html      # User captive portal (upload to MikroTik)
│   ├── admin.html      # Admin dashboard (upload to ESP SPIFFS)
│   ├── portal.js       # User portal logic (upload to MikroTik)
│   └── admin.js        # Admin dashboard logic (upload to ESP SPIFFS)
├── mikrotik/
│   └── setup.rsc       # RouterOS configuration script
├── esp/piso-wifi/
│   ├── piso-wifi.ino   # Main sketch
│   ├── config.h        # Constants & defaults
│   ├── eeprom_manager.h/.cpp   # Sales data persistence
│   ├── spiffs_manager.h/.cpp   # Config & file storage
│   ├── mikrotik_telnet.h/.cpp  # RouterOS Telnet CLI client
│   └── web_server.h/.cpp       # HTTP API endpoints
└── docs/plans/
    └── draft.md        # System design plan
```

## API Endpoints (ESP)

### Portal API
| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/lock` | Lock session to MAC, enable coin acceptor |
| GET | `/api/unlock` | Unlock session, disable coin acceptor |
| GET | `/api/coinStatus` | Get pulse count and coin amount |
| POST | `/api/finalize` | Finalize coin, update sales, create MikroTik session |
| GET | `/api/rates` | Get promo rates |

### Admin API (HTTP Basic Auth)
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/admin/api/dashboard` | Dashboard stats |
| GET | `/admin/api/resetStatistic` | Reset sales stats |
| GET | `/admin/api/getSystemConfig` | Get system config |
| POST | `/admin/api/saveSystemConfig` | Save system config |
| GET | `/admin/api/getRates` | Get promo rates |
| POST | `/admin/api/saveRates` | Save promo rates |
| POST | `/admin/api/generateVouchers` | Generate vouchers codes |
| POST | `/admin/updateMainBin` | OTA firmware update |

## Pin Assignment (Default)

| Pin | Function |
|-----|----------|
| D6 (GPIO12) | Coin slot pulse input |
| D7 (GPIO13) | Coin slot set/enable output |
| D3 (GPIO0) | System ready LED |
| D4 (GPIO2) | Insert coin LED |
| D5 (GPIO14) | Insert coin button (optional) |

## MikroTik Setup

1. Import `mikrotik/setup.rsc` via Terminal
2. Upload `index.html` and `portal.js` to MikroTik `Files/hotspot/`
3. Add ESP MAC to `IP -> Hotspot -> IP Bindings` (type: bypassed)
4. Create API user for ESP Telnet access (port 23)
5. Configure hotspot profile with minimal login/logout scripts

## ESP Build (Arduino IDE)

1. Install ESP8266 board support
2. Open `esp/piso-wifi/piso-wifi.ino`
3. Select board: NodeMCU 1.0 (ESP-12E Module)
4. Upload sketch
5. Upload SPIFFS data (if using separate filesystem upload)

## System Flow

1. User connects to WiFi -> MikroTik redirects to captive portal
2. User clicks "Insert Coin" -> Portal sends lock request to ESP
3. ESP enables coin acceptor, starts 60-second timer
4. User inserts coin -> ESP counts pulses, calculates time
5. ESP updates EEPROM (sales data)
6. ESP calls MikroTik API -> creates/extends hotspot session
7. User gets internet access
8. Session expires -> MikroTik disconnects user

## Notes

- Single-user system (one MAC locked at a time)
- 60-second coin window (resets on each new coin)
- No PHP backend, no cloud services, no Telegram notifications
- Standalone deployment (no multi-unit)
