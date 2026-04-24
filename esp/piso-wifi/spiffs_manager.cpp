#include "spiffs_manager.h"
#include "config.h"

SpiffsManager spiffsMgr;

static void setDefaultConfig(SystemConfig& cfg) {
  memset(&cfg, 0, sizeof(cfg));
  strlcpy(cfg.vendoName, "PisoWiFi", sizeof(cfg.vendoName));
  strlcpy(cfg.wifiSSID, "", sizeof(cfg.wifiSSID));
  strlcpy(cfg.wifiPW, "", sizeof(cfg.wifiPW));
  strlcpy(cfg.mtIp, "10.0.0.1", sizeof(cfg.mtIp));
  strlcpy(cfg.mtUser, "esptelnet", sizeof(cfg.mtUser));
  strlcpy(cfg.mtPw, "securepassword123", sizeof(cfg.mtPw));
  cfg.coinWaitTime = DEFAULT_COIN_WAIT_TIME;

  strlcpy(cfg.adminUser, "admin", sizeof(cfg.adminUser));
  strlcpy(cfg.adminPw, "admin", sizeof(cfg.adminPw));

  cfg.abuseCount = DEFAULT_ABUSE_COUNT;
  cfg.banMinutes = DEFAULT_BAN_MINUTES;

  cfg.pinCoinSlot = DEFAULT_COIN_SLOT_PIN;
  cfg.pinCoinSet = DEFAULT_COIN_SET_PIN;
  cfg.pinReadyLed = DEFAULT_READY_LED_PIN;
  cfg.pinInsertLed = DEFAULT_INSERT_LED_PIN;
  cfg.lcdScreen = 0;
  cfg.pinInsertBtn = DEFAULT_INSERT_BTN_PIN;

  cfg.checkInternet = 1;
  strlcpy(cfg.voucherPrefix, "PW", sizeof(cfg.voucherPrefix));
  strlcpy(cfg.welcomeMsg, "Welcome", sizeof(cfg.welcomeMsg));
  cfg.setupDone = 0;
  cfg.voucherLoginOpt = 0;
  strlcpy(cfg.voucherProfile, "default", sizeof(cfg.voucherProfile));
  cfg.voucherValidity = 0;

  cfg.ledTriggerHigh = DEFAULT_LED_TRIGGER_HIGH;

  cfg.ipMode = 0;
  strlcpy(cfg.localIp, "10.0.0.2", sizeof(cfg.localIp));
  strlcpy(cfg.gatewayIp, "10.0.0.1", sizeof(cfg.gatewayIp));
  strlcpy(cfg.subnetMask, "255.255.255.0", sizeof(cfg.subnetMask));
  strlcpy(cfg.dnsServer, "10.0.0.1", sizeof(cfg.dnsServer));
}

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
    setDefaultConfig(cfg);
    return true;
  }
  
  // Parse pipe-delimited string
  memset(&cfg, 0, sizeof(cfg));

  const size_t bufLen = (size_t)data.length() + 1;
  char* buf = (char*)malloc(bufLen);
  if (!buf) return false;
  data.toCharArray(buf, bufLen);
  char* token = strtok(buf, FIELD_SEP);

  auto nextStr = [&](char* dest, size_t len) {
    if (token) {
      strlcpy(dest, token, len);
      token = strtok(NULL, FIELD_SEP);
    }
  };
  auto nextU8 = [&]() -> uint8_t {
    uint8_t v = token ? (uint8_t)atoi(token) : 0;
    if (token) token = strtok(NULL, FIELD_SEP);
    return v;
  };
  auto nextU16 = [&]() -> uint16_t {
    uint16_t v = token ? (uint16_t)atoi(token) : 0;
    if (token) token = strtok(NULL, FIELD_SEP);
    return v;
  };

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
  cfg.pinCoinSet = nextU8();
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

  free(buf);

  // Minimal fallbacks for older/partial configs
  if (cfg.vendoName[0] == '\0') strlcpy(cfg.vendoName, "PisoWiFi", sizeof(cfg.vendoName));
  if (cfg.mtIp[0] == '\0') strlcpy(cfg.mtIp, "10.0.0.1", sizeof(cfg.mtIp));
  if (cfg.mtUser[0] == '\0') strlcpy(cfg.mtUser, "admin", sizeof(cfg.mtUser));
  if (cfg.adminUser[0] == '\0') strlcpy(cfg.adminUser, "admin", sizeof(cfg.adminUser));
  if (cfg.adminPw[0] == '\0') strlcpy(cfg.adminPw, "admin", sizeof(cfg.adminPw));
  if (cfg.voucherPrefix[0] == '\0') strlcpy(cfg.voucherPrefix, "PW", sizeof(cfg.voucherPrefix));
  if (cfg.welcomeMsg[0] == '\0') strlcpy(cfg.welcomeMsg, "Welcome", sizeof(cfg.welcomeMsg));
  if (cfg.voucherProfile[0] == '\0') strlcpy(cfg.voucherProfile, "default", sizeof(cfg.voucherProfile));
  
  return true;
}

bool SpiffsManager::saveConfig(const SystemConfig& cfg) {
  if (!begin()) return false;
  String data;
  data.reserve(512);

  data += String(cfg.vendoName) + FIELD_SEP;
  data += String(cfg.wifiSSID) + FIELD_SEP;
  data += String(cfg.wifiPW) + FIELD_SEP;
  data += String(cfg.mtIp) + FIELD_SEP;
  data += String(cfg.mtUser) + FIELD_SEP;
  data += String(cfg.mtPw) + FIELD_SEP;
  data += String(cfg.coinWaitTime) + FIELD_SEP;

  data += String(cfg.adminUser) + FIELD_SEP;
  data += String(cfg.adminPw) + FIELD_SEP;

  data += String(cfg.abuseCount) + FIELD_SEP;
  data += String(cfg.banMinutes) + FIELD_SEP;

  data += String(cfg.pinCoinSlot) + FIELD_SEP;
  data += String(cfg.pinCoinSet) + FIELD_SEP;
  data += String(cfg.pinReadyLed) + FIELD_SEP;
  data += String(cfg.pinInsertLed) + FIELD_SEP;
  data += String(cfg.lcdScreen) + FIELD_SEP;
  data += String(cfg.pinInsertBtn) + FIELD_SEP;

  data += String(cfg.checkInternet) + FIELD_SEP;
  data += String(cfg.voucherPrefix) + FIELD_SEP;
  data += String(cfg.welcomeMsg) + FIELD_SEP;
  data += String(cfg.setupDone) + FIELD_SEP;
  data += String(cfg.voucherLoginOpt) + FIELD_SEP;
  data += String(cfg.voucherProfile) + FIELD_SEP;
  data += String(cfg.voucherValidity) + FIELD_SEP;
  data += String(cfg.ledTriggerHigh) + FIELD_SEP;

  data += String(cfg.ipMode) + FIELD_SEP;
  data += String(cfg.localIp) + FIELD_SEP;
  data += String(cfg.gatewayIp) + FIELD_SEP;
  data += String(cfg.subnetMask) + FIELD_SEP;
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
