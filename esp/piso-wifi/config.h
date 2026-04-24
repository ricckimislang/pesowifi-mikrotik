#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===== VERSION =====
#define FIRMWARE_VERSION "1.0.0"

// ===== DEFAULT PINS (ESP8266 NodeMCU) =====
#define DEFAULT_COIN_SLOT_PIN     12   // D6
#define DEFAULT_COIN_SET_PIN      13   // D7
#define DEFAULT_READY_LED_PIN      0   // D3
#define DEFAULT_INSERT_LED_PIN     2   // D4
#define DEFAULT_INSERT_BTN_PIN    14   // D5
#define DEFAULT_LED_TRIGGER_HIGH   1   // HIGH = 1

// ===== DEFAULT TIMINGS =====
#define DEFAULT_COIN_WAIT_TIME    60   // seconds
#define DEFAULT_ABUSE_COUNT        3
#define DEFAULT_BAN_MINUTES        5

// ===== EEPROM ADDRESSES =====
#define EEPROM_ADDR_INCOME      0   // 4 bytes (uint32)
#define EEPROM_ADDR_COINS       4   // 4 bytes (uint32)
#define EEPROM_ADDR_CUSTOMERS   8   // 4 bytes (uint32)
#define EEPROM_ADDR_CHECKSUM   12   // 4 bytes
#define EEPROM_BACKUP_LEN      16   // 4 bytes (length of backup data)
#define EEPROM_BACKUP_START    20   // Start of backup data (up to ~200 bytes)
#define EEPROM_SIZE            256

// ===== NETWORK =====
#define DEFAULT_ESP_IP         IPAddress(10, 0, 0, 2)
#define DEFAULT_GATEWAY        IPAddress(10, 0, 0, 1)
#define DEFAULT_SUBNET         IPAddress(255, 255, 255, 0)
#define DEFAULT_DNS            IPAddress(8, 8, 8, 8)

// ===== WIFI =====
#define WIFI_CONNECT_TIMEOUT    30000  // ms
#define AP_SSID_PREFIX          "PisoWiFi-Setup"

// ===== WEB SERVER =====
#define HTTP_PORT               80
#define ADMIN_SESSION_TIMEOUT   300000 // 5 minutes

// ===== MIKROTIK TELNET =====
#define MT_TELNET_PORT          23
#define MT_TELNET_TIMEOUT       5000  // ms

// ===== INTERNET CHECK =====
#define INTERNET_CHECK_URL      "http://ifconfig.me"
#define INTERNET_CHECK_TIMEOUT  10000  // ms

// ===== SEPARATORS (must match portal) =====
#define FIELD_SEP  "|"
#define ROW_SEP    "#"

// ===== FILE PATHS (SPIFFS) =====
#define PATH_SYSTEM_CONFIG  "/config.sys"
#define PATH_RATES          "/rates.cfg"
#define PATH_VOUCHERS       "/vouchers.dat"
#define PATH_LOG            "/log.txt"

#endif
