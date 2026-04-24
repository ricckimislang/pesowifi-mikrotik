#include "spiffs_manager.h"

SpiffsManager spiffsMgr;

bool SpiffsManager::begin() {
  if (!initialized) {
    initialized = SPIFFS.begin();
  }
  return initialized;
}

bool SpiffsManager::loadConfig(SystemConfig& cfg) {
  if (!begin()) return false;
  String data = readFile(PATH_SYSTEM_CONFIG);
  if (data.length() == 0) {
    // Return defaults
    memset(&cfg, 0, sizeof(cfg));
    strlcpy(cfg.vendoName, "PisoWiFi", sizeof(cfg.vendoName));
    strlcpy(cfg.wifiSSID, "", sizeof(cfg.wifiSSID));
    strlcpy(cfg.wifiPW, "", sizeof(cfg.wifiPW));
    strlcpy(cfg.mtIp, "10.0.0.1", sizeof(cfg.mtIp));
    strlcpy(cfg.mtUser, "admin", sizeof(cfg.mtUser));
    strlcpy(cfg.mtPw, "", sizeof(cfg.mtPw));
    cfg.coinWaitTime = DEFAULT_COIN_WAIT_TIME;
    strlcpy(cfg.adminUser, "admin", sizeof(cfg.adminUser));
    strlcpy(cfg.adminPw, "admin", sizeof(cfg.adminPw));
    cfg.abuseCount = DEFAULT_ABUSE_COUNT;
    cfg.banMinutes = DEFAULT_BAN_MINUTES;
    cfg.pinCoinSlot = DEFAULT_COIN_SLOT_PIN;
    cfg.pinCoinSet = DEFAULT_COIN_SET_PIN;
    cfg.pinReadyLed = DEFAULT_READY_LED_PIN;
    cfg.pinInsertLed = DEFAULT_INSERT_LED_PIN;
    cfg.pinInsertBtn = DEFAULT_INSERT_BTN_PIN;
    cfg.ledTriggerHigh = DEFAULT_LED_TRIGGER_HIGH;
    cfg.ipMode = 0;
    strlcpy(cfg.localIp, "10.0.0.2", sizeof(cfg.localIp));
    strlcpy(cfg.gatewayIp, "10.0.0.1", sizeof(cfg.gatewayIp));
    strlcpy(cfg.subnetMask, "255.255.255.0", sizeof(cfg.subnetMask));
    strlcpy(cfg.dnsServer, "8.8.8.8", sizeof(cfg.dnsServer));
    strlcpy(cfg.voucherPrefix, "PW", sizeof(cfg.voucherPrefix));
    cfg.setupDone = 0;
    return true;
  }
  
  // Parse pipe-delimited string
  int idx = 0;
  char buf[256];
  data.toCharArray(buf, sizeof(buf));
  char* token = strtok(buf, "|");
  
  auto next = [&](char* dest, size_t len) {
    if (token) {
      strlcpy(dest, token, len);
      token = strtok(NULL, "|");
    }
  };
  
  next(cfg.vendoName, sizeof(cfg.vendoName));
  next(cfg.wifiSSID, sizeof(cfg.wifiSSID));
  next(cfg.wifiPW, sizeof(cfg.wifiPW));
  next(cfg.mtIp, sizeof(cfg.mtIp));
  next(cfg.mtUser, sizeof(cfg.mtUser));
  next(cfg.mtPw, sizeof(cfg.mtPw));
  
  if (token) cfg.coinWaitTime = atoi(token);
  token = strtok(NULL, "|");
  
  next(cfg.adminUser, sizeof(cfg.adminUser));
  next(cfg.adminPw, sizeof(cfg.adminPw));
  
  if (token) cfg.abuseCount = atoi(token);
  token = strtok(NULL, "|");
  if (token) cfg.banMinutes = atoi(token);
  token = strtok(NULL, "|");
  if (token) cfg.pinCoinSlot = atoi(token);
  token = strtok(NULL, "|");
  if (token) cfg.pinCoinSet = atoi(token);
  token = strtok(NULL, "|");
  if (token) cfg.pinReadyLed = atoi(token);
  token = strtok(NULL, "|");
  if (token) cfg.pinInsertLed = atoi(token);
  token = strtok(NULL, "|");
  
  // skip lcdScreen
  token = strtok(NULL, "|");
  
  if (token) cfg.pinInsertBtn = atoi(token);
  token = strtok(NULL, "|");
  
  // skip checkInternet, voucherPrefix, welcomeMsg, setupDone, voucherLoginOpt, voucherProfile, voucherValidity, ledTrigger
  for (int i = 0; i < 9 && token; i++) token = strtok(NULL, "|");
  
  if (token) cfg.ipMode = atoi(token);
  token = strtok(NULL, "|");
  next(cfg.localIp, sizeof(cfg.localIp));
  next(cfg.gatewayIp, sizeof(cfg.gatewayIp));
  next(cfg.subnetMask, sizeof(cfg.subnetMask));
  next(cfg.dnsServer, sizeof(cfg.dnsServer));
  
  return true;
}

bool SpiffsManager::saveConfig(const SystemConfig& cfg) {
  if (!begin()) return false;
  String data;
  data += String(cfg.vendoName) + "|";
  data += String(cfg.wifiSSID) + "|";
  data += String(cfg.wifiPW) + "|";
  data += String(cfg.mtIp) + "|";
  data += String(cfg.mtUser) + "|";
  data += String(cfg.mtPw) + "|";
  data += String(cfg.coinWaitTime) + "|";
  data += String(cfg.adminUser) + "|";
  data += String(cfg.adminPw) + "|";
  data += String(cfg.abuseCount) + "|";
  data += String(cfg.banMinutes) + "|";
  data += String(cfg.pinCoinSlot) + "|";
  data += String(cfg.pinCoinSet) + "|";
  data += String(cfg.pinReadyLed) + "|";
  data += String(cfg.pinInsertLed) + "|";
  data += "0|"; // lcdScreen
  data += String(cfg.pinInsertBtn) + "|";
  data += "1|"; // checkInternet
  data += String(cfg.voucherPrefix) + "|";
  data += "Welcome|"; // welcomeMsg
  data += String(cfg.setupDone) + "|";
  data += "0|"; // voucherLoginOpt
  data += "default|"; // voucherProfile
  data += "0|"; // voucherValidity
  data += String(cfg.ledTriggerHigh) + "|";
  data += String(cfg.ipMode) + "|";
  data += String(cfg.localIp) + "|";
  data += String(cfg.gatewayIp) + "|";
  data += String(cfg.subnetMask) + "|";
  data += String(cfg.dnsServer);
  
  return writeFile(PATH_SYSTEM_CONFIG, data);
}

String SpiffsManager::readFile(const char* path) {
  if (!begin()) return "";
  File f = SPIFFS.open(path, "r");
  if (!f) return "";
  String data = f.readString();
  f.close();
  return data;
}

bool SpiffsManager::writeFile(const char* path, const String& data) {
  if (!begin()) return false;
  File f = SPIFFS.open(path, "w");
  if (!f) return false;
  f.print(data);
  f.close();
  return true;
}

bool SpiffsManager::appendFile(const char* path, const String& data) {
  if (!begin()) return false;
  File f = SPIFFS.open(path, "a");
  if (!f) return false;
  f.print(data);
  f.close();
  return true;
}

bool SpiffsManager::exists(const char* path) {
  if (!begin()) return false;
  return SPIFFS.exists(path);
}

bool SpiffsManager::remove(const char* path) {
  if (!begin()) return false;
  return SPIFFS.remove(path);
}

void SpiffsManager::log(const char* msg) {
  String line = String(millis() / 1000) + " " + msg + "\n";
  appendFile(PATH_LOG, line);
}
