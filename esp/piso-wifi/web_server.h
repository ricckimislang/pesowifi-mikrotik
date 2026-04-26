#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <base64.h>
#include "config.h"
#include "eeprom_manager.h"
#include "spiffs_manager.h"
#include "mikrotik_telnet.h"

struct MacAttempt {
  String mac;
  long unlockTime;
  int attemptCount;
};

#define MAX_RATE_LIMIT_IPS 10
struct RateLimitEntry {
  String ip;
  int requestCount;
  unsigned long windowStart;
};

class WebServerMgr {
public:
  void begin();
  void handleClient();
  
private:
  ESP8266WebServer server{HTTP_PORT};
  ESP8266HTTPUpdateServer updateServer;
  String adminAuth = ""; // base64(user:pass)
  
  RateLimitEntry rateLimits[MAX_RATE_LIMIT_IPS];
  
  void setupRoutes();
  void sendCORS();
  void sendJson(const String& json);
  void sendPlain(const String& text);
  void sendError(int code, const String& msg);
  void handleServeFile(const char* path, const char* contentType);
  
  // Portal API
  void handleLock();
  void handleUnlock();
  void handleCoinStatus();
  void handleFinalize();
  void handleRates();
  
  // Admin API
  void handleAdminLogin();
  void handleAdminLogout();
  void handleDashboard();
  void handleResetStats();
  void handleGetSystemConfig();
  void handleSaveSystemConfig();
  void handleGetRates();
  void handleSaveRates();
  void handleGenerateVouchers();
  void handleUpdateFirmware();
  
  // Helpers
  bool isAuthorized();
  void handleNotAuthorized();
  bool checkMacAbuse(const String& mac);
  void addMacAttempt(const String& mac);
  void clearMacAttempt(const String& mac);
  bool hasInternetConnection();
  void backupSystemConfig();
};

extern WebServerMgr webServer;

#endif
