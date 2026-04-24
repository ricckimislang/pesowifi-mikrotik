/*
  Piso WiFi Vendo - ESP8266 Firmware
  Standalone coin-operated WiFi hotspot controller
  
  Hardware: ESP8266 NodeMCU, Coin Acceptor, MikroTik hAP Lite
  Storage: EEPROM (sales data), SPIFFS (config/rates/vouchers)
  
  Pin defaults:
    D6 (GPIO12) - Coin slot pulse input
    D7 (GPIO13) - Coin slot set/enable output
    D3 (GPIO0)  - System ready LED
    D4 (GPIO2)  - Insert coin LED
    D5 (GPIO14) - Insert coin button (optional)
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "eeprom_manager.h"
#include "spiffs_manager.h"
#include "mikrotik_telnet.h"
#include "web_server.h"

// ===== GLOBAL STATE =====
SystemConfig sysCfg;

volatile uint32_t pulseCount = 0;
volatile uint32_t coinAmount = 0;
volatile bool coinWindowOpen = false;
volatile bool sessionLocked = false;

String lockedMac = "";
String lockedIp = "";
volatile unsigned long coinWindowStart = 0;
uint16_t coinWaitTimeSec = DEFAULT_COIN_WAIT_TIME;

bool readyLedState = false;
bool insertLedState = false;
bool sysReady = false;
bool otaEnabled = false;

// ===== INTERRUPT HANDLER =====
void ICACHE_RAM_ATTR onCoinPulse() {
  if (!coinWindowOpen) return;
  pulseCount++;
  // Simple mapping: each pulse = 1 PHP (adjust based on your coin acceptor)
  coinAmount = pulseCount;
  // Reset window timer on each pulse
  coinWindowStart = millis();
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[PisoWiFi] Booting...");
  randomSeed(ESP.getChipId() ^ micros());

  // Init storage
  eepromMgr.begin();
  spiffsMgr.begin();
  spiffsMgr.log("Boot");

  // Check for config backup (after OTA)
  String backup = eepromMgr.restoreConfig();
  if (backup.length() > 0) {
    spiffsMgr.writeFile(PATH_SYSTEM_CONFIG, backup);
    eepromMgr.clearBackup();
    spiffsMgr.log("Config restored from backup, rebooting...");
    delay(1000);
    ESP.restart();
    return;
  }

  // Load config
  spiffsMgr.loadConfig(sysCfg);
  coinWaitTimeSec = sysCfg.coinWaitTime > 0 ? sysCfg.coinWaitTime : DEFAULT_COIN_WAIT_TIME;

  // Init pins
  pinMode(sysCfg.pinCoinSlot, INPUT_PULLUP);
  pinMode(sysCfg.pinCoinSet, OUTPUT);
  digitalWrite(sysCfg.pinCoinSet, sysCfg.ledTriggerHigh ? LOW : HIGH); // disabled

  pinMode(sysCfg.pinReadyLed, OUTPUT);
  digitalWrite(sysCfg.pinReadyLed, sysCfg.ledTriggerHigh ? LOW : HIGH);

  pinMode(sysCfg.pinInsertLed, OUTPUT);
  digitalWrite(sysCfg.pinInsertLed, sysCfg.ledTriggerHigh ? LOW : HIGH);

  // Attach interrupt for coin pulse
  attachInterrupt(digitalPinToInterrupt(sysCfg.pinCoinSlot), onCoinPulse, FALLING);

  // WiFi
  initWiFi();

  // OTA
  if (strlen(sysCfg.adminPw) > 0) {
    ArduinoOTA.setHostname("piso-wifi");
    ArduinoOTA.setPassword(sysCfg.adminPw);
    ArduinoOTA.begin();
    otaEnabled = true;
    spiffsMgr.log("ArduinoOTA enabled");
  } else {
    otaEnabled = false;
    spiffsMgr.log("ArduinoOTA disabled: adminPw empty");
  }

  // Web server
  webServer.begin();

  // mDNS
  MDNS.begin("piso-wifi");

  sysReady = true;
  spiffsMgr.log("Ready. IP: " + WiFi.localIP().toString());
  Serial.println("[PisoWiFi] Ready. IP: " + WiFi.localIP().toString());
}

// ===== LOOP =====
void loop() {
  ESP.wdtFeed();
  if (otaEnabled) ArduinoOTA.handle();
  webServer.handleClient();
  MDNS.update();

  // Check coin window timeout
  if (coinWindowOpen && sessionLocked) {
    unsigned long elapsed = (millis() - coinWindowStart) / 1000;
    if (elapsed >= coinWaitTimeSec) {
      // Timeout - auto-finalize if pulses detected, else unlock
      if (pulseCount > 0) {
        // Auto finalize handled by portal or next loop
      } else {
        sessionLocked = false;
        coinWindowOpen = false;
        lockedMac = "";
        lockedIp = "";
        digitalWrite(sysCfg.pinCoinSet, sysCfg.ledTriggerHigh ? LOW : HIGH);
        digitalWrite(sysCfg.pinInsertLed, sysCfg.ledTriggerHigh ? LOW : HIGH);
        spiffsMgr.log("Coin window timeout - no coin");
      }
    }
  }

  // Update LEDs
  updateLeds();

  delay(10);
}

// ===== WIFI =====
void initWiFi() {
  if (strlen(sysCfg.wifiSSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(sysCfg.wifiSSID, sysCfg.wifiPW);

    if (sysCfg.ipMode == 1) {
      IPAddress local, gw, sn, dns;
      local.fromString(sysCfg.localIp);
      gw.fromString(sysCfg.gatewayIp);
      sn.fromString(sysCfg.subnetMask);
      dns.fromString(sysCfg.dnsServer);
      WiFi.config(local, gw, sn, dns);
    }

    Serial.print("[WiFi] Connecting");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi] Connected: " + WiFi.localIP().toString());
      return;
    }
  }

  // Fallback: setup AP for configuration
  WiFi.mode(WIFI_AP_STA);
  String apName = String(AP_SSID_PREFIX) + "-" + String(ESP.getChipId(), HEX);
  String apPass = "PW" + String(ESP.getChipId(), HEX);
  apPass.toUpperCase();
  while (apPass.length() < 8) apPass += "0";
  WiFi.softAP(apName.c_str(), apPass.c_str());
  Serial.println("[WiFi] AP mode: " + apName + " / " + apPass);
}

// ===== LEDS =====
void updateLeds() {
  // Ready LED: blink slowly when system is ready, solid when coin window open
  if (sessionLocked && coinWindowOpen) {
    // Solid insert LED
    digitalWrite(sysCfg.pinInsertLed, sysCfg.ledTriggerHigh ? HIGH : LOW);
    // Blink ready LED fast
    bool fastBlink = (millis() / 200) % 2 == 0;
    digitalWrite(sysCfg.pinReadyLed, fastBlink ? (sysCfg.ledTriggerHigh ? HIGH : LOW) : (sysCfg.ledTriggerHigh ? LOW : HIGH));
  } else if (sysReady) {
    // Slow blink ready LED
    bool slowBlink = (millis() / 1000) % 2 == 0;
    digitalWrite(sysCfg.pinReadyLed, slowBlink ? (sysCfg.ledTriggerHigh ? HIGH : LOW) : (sysCfg.ledTriggerHigh ? LOW : HIGH));
    digitalWrite(sysCfg.pinInsertLed, sysCfg.ledTriggerHigh ? LOW : HIGH);
  } else {
    // Not ready - both off
    digitalWrite(sysCfg.pinReadyLed, sysCfg.ledTriggerHigh ? LOW : HIGH);
    digitalWrite(sysCfg.pinInsertLed, sysCfg.ledTriggerHigh ? LOW : HIGH);
  }
}
