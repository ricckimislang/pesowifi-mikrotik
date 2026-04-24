#ifndef SPIFFS_MANAGER_H
#define SPIFFS_MANAGER_H

#include <Arduino.h>
#include <FS.h>

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
  uint8_t pinInsertBtn;
  uint8_t ledTriggerHigh;
  uint8_t ipMode;        // 0=DHCP, 1=Static
  char localIp[16];
  char gatewayIp[16];
  char subnetMask[16];
  char dnsServer[16];
  char voucherPrefix[4];
  uint8_t setupDone;
};

class SpiffsManager {
public:
  bool begin();
  bool loadConfig(SystemConfig& cfg);
  bool saveConfig(const SystemConfig& cfg);
  String readFile(const char* path);
  bool writeFile(const char* path, const String& data);
  bool appendFile(const char* path, const String& data);
  bool exists(const char* path);
  bool remove(const char* path);
  void log(const char* msg);
private:
  bool initialized = false;
};

extern SpiffsManager spiffsMgr;

#endif
