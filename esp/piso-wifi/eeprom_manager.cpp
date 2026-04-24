#include "eeprom_manager.h"

EepromManager eepromMgr;

void EepromManager::begin() {
  if (!initialized) {
    EEPROM.begin(EEPROM_SIZE);
    initialized = true;
    SalesData data = read();
    if (data.lifetimeIncome == 0xFFFFFFFF && data.coinTotal == 0xFFFFFFFF) {
      // First boot - initialize with zeros
      SalesData empty = {0, 0, 0};
      write(empty);
    }
  }
}

SalesData EepromManager::read() {
  begin();
  SalesData data;
  EEPROM.get(EEPROM_ADDR_INCOME, data.lifetimeIncome);
  EEPROM.get(EEPROM_ADDR_COINS, data.coinTotal);
  EEPROM.get(EEPROM_ADDR_CUSTOMERS, data.customerCount);
  
  uint32_t storedChecksum;
  EEPROM.get(EEPROM_ADDR_CHECKSUM, storedChecksum);
  
  uint32_t calcChecksum = getChecksum(data);
  if (storedChecksum != calcChecksum) {
    // Corrupted or uninitialized - return zeros
    data.lifetimeIncome = 0;
    data.coinTotal = 0;
    data.customerCount = 0;
  }
  return data;
}

void EepromManager::write(const SalesData& data) {
  begin();
  EEPROM.put(EEPROM_ADDR_INCOME, data.lifetimeIncome);
  EEPROM.put(EEPROM_ADDR_COINS, data.coinTotal);
  EEPROM.put(EEPROM_ADDR_CUSTOMERS, data.customerCount);
  
  uint32_t checksum = getChecksum(data);
  EEPROM.put(EEPROM_ADDR_CHECKSUM, checksum);
  EEPROM.commit();
}

void EepromManager::reset(const char* type) {
  SalesData data = read();
  if (strcmp(type, "lifeTimeCount") == 0) {
    data.lifetimeIncome = 0;
  } else if (strcmp(type, "coinCount") == 0) {
    data.coinTotal = 0;
  } else if (strcmp(type, "customerCount") == 0) {
    data.customerCount = 0;
  }
  write(data);
}

void EepromManager::backupConfig(const char* configData) {
  begin();
  int len = strlen(configData);
  if (len > 200) len = 200; // limit to available space
  EEPROM.put(EEPROM_BACKUP_LEN, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(EEPROM_BACKUP_START + i, configData[i]);
  }
  EEPROM.commit();
}

String EepromManager::restoreConfig() {
  begin();
  int len = 0;
  EEPROM.get(EEPROM_BACKUP_LEN, len);
  if (len <= 0 || len > 200) return "";
  String data = "";
  for (int i = 0; i < len; i++) {
    char c = EEPROM.read(EEPROM_BACKUP_START + i);
    data += c;
  }
  return data;
}

void EepromManager::clearBackup() {
  begin();
  EEPROM.put(EEPROM_BACKUP_LEN, 0);
  EEPROM.commit();
}

uint32_t EepromManager::getChecksum(const SalesData& data) {
  return data.lifetimeIncome + data.coinTotal + data.customerCount + 0xDEADBEEF;
}

bool EepromManager::validate(const SalesData& data, uint32_t checksum) {
  return checksum == getChecksum(data);
}
