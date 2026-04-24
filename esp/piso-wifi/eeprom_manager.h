#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

struct SalesData {
  uint32_t lifetimeIncome;
  uint32_t coinTotal;
  uint32_t customerCount;
};

class EepromManager {
public:
  void begin();
  SalesData read();
  void write(const SalesData& data);
  void reset(const char* type);
  uint32_t getChecksum(const SalesData& data);
  bool validate(const SalesData& data, uint32_t checksum);
  
  // Config backup for OTA
  void backupConfig(const char* configData);
  String restoreConfig();
  void clearBackup();
private:
  bool initialized = false;
};

extern EepromManager eepromMgr;

#endif
