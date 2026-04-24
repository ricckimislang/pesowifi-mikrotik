#include "web_server.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

WebServerMgr webServer;

// External globals declared in main sketch
extern SystemConfig sysCfg;
extern volatile uint32_t pulseCount;
extern volatile uint32_t coinAmount;
extern volatile bool coinWindowOpen;
extern volatile bool sessionLocked;
extern String lockedMac;
extern String lockedIp;
extern unsigned long coinWindowStart;
extern uint16_t coinWaitTimeSec;

// MAC abuse tracking
#define MAX_MAC_ATTEMPTS 20
MacAttempt macAttempts[MAX_MAC_ATTEMPTS];

void WebServerMgr::begin() {
  setupRoutes();
  // Require Basic Auth for HTTP OTA update endpoint (/update)
  const bool hasUpdateCreds = (strlen(sysCfg.adminUser) > 0) && (strlen(sysCfg.adminPw) > 0);
  if (hasUpdateCreds) {
    updateServer.setup(&server, "/update", sysCfg.adminUser, sysCfg.adminPw);
  } else {
    spiffsMgr.log("HTTP OTA disabled: adminUser/adminPw not set");
  }
  server.begin();
  spiffsMgr.log("Web server started");
}

void WebServerMgr::handleClient() {
  server.handleClient();
}

void WebServerMgr::sendCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

void WebServerMgr::sendPlain(const String& text) {
  sendCORS();
  server.send(200, "text/plain", text);
}

void WebServerMgr::sendJson(const String& json) {
  sendCORS();
  server.send(200, "application/json", json);
}

void WebServerMgr::sendError(int code, const String& msg) {
  sendCORS();
  server.send(code, "text/plain", msg);
}

void WebServerMgr::handleServeFile(const char* path, const char* contentType) {
  String data = spiffsMgr.readFile(path);
  if (data.length() == 0) {
    server.send(404, "text/plain", "File not found: " + String(path));
    return;
  }
  sendCORS();
  server.send(200, contentType, data);
}

bool WebServerMgr::isAuthorized() {
  String auth = server.header("Authorization");
  if (!auth.startsWith("Basic ")) return false;
  String expectedAuth = "Basic " + adminAuth;
  return auth.equals(expectedAuth);
}

void WebServerMgr::handleNotAuthorized() {
  server.sendHeader("WWW-Authenticate", "Basic realm=\"PisoWiFi Admin\"");
  server.send(401, "text/html", "<html><body><h1>401 Unauthorized</h1></body></html>");
}

bool WebServerMgr::checkMacAbuse(const String& mac) {
  if (sysCfg.abuseCount == 0) return true;
  for (int i = 0; i < MAX_MAC_ATTEMPTS; i++) {
    if (macAttempts[i].mac == mac) {
      if (macAttempts[i].attemptCount >= sysCfg.abuseCount) {
        long now = millis();
        if (macAttempts[i].unlockTime > 0 && macAttempts[i].unlockTime <= now) {
          // Ban expired, clear it
          macAttempts[i].mac = "";
          macAttempts[i].attemptCount = 0;
          macAttempts[i].unlockTime = 0;
          return true;
        }
        return false; // Still banned
      }
    }
  }
  return true;
}

void WebServerMgr::addMacAttempt(const String& mac) {
  if (sysCfg.abuseCount == 0) return;
  int idx = -1;
  int freeIdx = -1;
  for (int i = 0; i < MAX_MAC_ATTEMPTS; i++) {
    if (macAttempts[i].mac == mac) {
      idx = i;
      break;
    } else if (macAttempts[i].mac == "" && freeIdx == -1) {
      freeIdx = i;
    }
  }
  if (idx >= 0) {
    macAttempts[idx].attemptCount++;
    if (macAttempts[idx].attemptCount >= sysCfg.abuseCount) {
      macAttempts[idx].unlockTime = millis() + (sysCfg.banMinutes * 60000);
    }
  } else if (freeIdx >= 0) {
    macAttempts[freeIdx].mac = mac;
    macAttempts[freeIdx].attemptCount = 1;
    macAttempts[freeIdx].unlockTime = 0;
  }
}

void WebServerMgr::clearMacAttempt(const String& mac) {
  for (int i = 0; i < MAX_MAC_ATTEMPTS; i++) {
    if (macAttempts[i].mac == mac) {
      macAttempts[i].mac = "";
      macAttempts[i].attemptCount = 0;
      macAttempts[i].unlockTime = 0;
      break;
    }
  }
}

bool WebServerMgr::hasInternetConnection() {
  HTTPClient http;
  http.begin(INTERNET_CHECK_URL);
  http.addHeader("User-Agent", "curl/7.55.1");
  int httpCode = http.GET();
  http.end();
  return httpCode > 0;
}

void WebServerMgr::backupSystemConfig() {
  String config = spiffsMgr.readFile(PATH_SYSTEM_CONFIG);
  if (config.length() > 0) {
    eepromMgr.backupConfig(config.c_str());
  }
}

void WebServerMgr::setupRoutes() {
  // Initialize admin auth
  String authStr = String(sysCfg.adminUser) + ":" + String(sysCfg.adminPw);
  adminAuth = base64::encode(authStr);
  
  // Portal API
  server.on("/api/lock", HTTP_POST, [this]() { handleLock(); });
  server.on("/api/unlock", HTTP_GET, [this]() { handleUnlock(); });
  server.on("/api/coinStatus", HTTP_GET, [this]() { handleCoinStatus(); });
  server.on("/api/finalize", HTTP_POST, [this]() { handleFinalize(); });
  server.on("/api/rates", HTTP_GET, [this]() { handleRates(); });
  
  // Admin API (Basic Auth)
  server.on("/admin/api/dashboard", HTTP_GET, [this]() { handleDashboard(); });
  server.on("/admin/api/resetStatistic", HTTP_GET, [this]() { handleResetStats(); });
  server.on("/admin/api/getSystemConfig", HTTP_GET, [this]() { handleGetSystemConfig(); });
  server.on("/admin/api/saveSystemConfig", HTTP_POST, [this]() { handleSaveSystemConfig(); });
  server.on("/admin/api/getRates", HTTP_GET, [this]() { handleGetRates(); });
  server.on("/admin/api/saveRates", HTTP_POST, [this]() { handleSaveRates(); });
  server.on("/admin/api/generateVouchers", HTTP_POST, [this]() { handleGenerateVouchers(); });
  server.on("/admin/updateMainBin", HTTP_POST, [this]() { handleUpdateFirmware(); });
  
  // Admin portal static files (served from ESP SPIFFS)
  server.on("/admin", HTTP_GET, [this]() {
    server.sendHeader("Location", "/admin.html");
    server.send(302, "text/plain", "");
  });
  server.on("/admin.html", HTTP_GET, [this]() { handleServeFile("/admin.html", "text/html"); });
  server.on("/admin.js", HTTP_GET, [this]() { handleServeFile("/admin.js", "application/javascript"); });
}

void WebServerMgr::handleLock() {
  sendCORS();
  if (sessionLocked) {
    sendError(409, "Session already locked");
    return;
  }
  String mac = server.arg("mac");
  String ip = server.arg("ip");
  if (mac.length() == 0) {
    sendError(400, "MAC required");
    return;
  }
  
  // Check MAC abuse
  if (!checkMacAbuse(mac)) {
    sendError(429, "MAC banned");
    return;
  }
  
  // Check internet (optional)
  if (sysCfg.checkInternet && !hasInternetConnection()) {
    sendError(503, "No internet");
    return;
  }
  
  lockedMac = mac;
  lockedIp = ip;
  sessionLocked = true;
  coinWindowOpen = true;
  coinWindowStart = millis();
  pulseCount = 0;
  coinAmount = 0;
  // Enable coin acceptor
  digitalWrite(sysCfg.pinCoinSet, sysCfg.ledTriggerHigh ? HIGH : LOW);
  spiffsMgr.log("Lock session: " + mac);
  sendPlain("OK");
}

void WebServerMgr::handleUnlock() {
  sendCORS();
  sessionLocked = false;
  coinWindowOpen = false;
  String mac = lockedMac;
  lockedMac = "";
  lockedIp = "";
  pulseCount = 0;
  coinAmount = 0;
  digitalWrite(sysCfg.pinCoinSet, sysCfg.ledTriggerHigh ? LOW : HIGH);
  if (mac.length() > 0) clearMacAttempt(mac);
  sendPlain("OK");
}

void WebServerMgr::handleCoinStatus() {
  sendCORS();
  String resp = String(pulseCount) + "|" + String(coinAmount);
  sendPlain(resp);
}

void WebServerMgr::handleFinalize() {
  sendCORS();
  
  // Extract and validate MAC from request
  String requestMac = server.arg("mac");
  if (requestMac.length() == 0) {
    sendError(400, "MAC address required");
    return;
  }
  
  // Validate MAC matches locked session
  if (!sessionLocked || requestMac != lockedMac) {
    sendError(403, "Invalid session or MAC mismatch");
    return;
  }
  
  if (pulseCount == 0) {
    sendError(400, "No coin to finalize");
    return;
  }
  
  // Map pulses to minutes using promo rates
  String ratesData = spiffsMgr.readFile(PATH_RATES);
  uint16_t minutes = 0;
  if (ratesData.length() > 0) {
    // Find matching rate by coin amount (PHP)
    int idx = 0;
    char buf[512];
    ratesData.toCharArray(buf, sizeof(buf));
    char* row = strtok(buf, ROW_SEP);
    while (row) {
      char* parts[6];
      int pidx = 0;
      char* tok = strtok(row, "|");
      while (tok && pidx < 6) {
        parts[pidx++] = tok;
        tok = strtok(NULL, "|");
      }
      if (pidx >= 3) {
        int price = atoi(parts[1]);
        int mins = atoi(parts[2]);
        if (price == (int)coinAmount) {
          minutes = mins;
          break;
        }
      }
      row = strtok(NULL, ROW_SEP);
    }
  }
  if (minutes == 0) {
    // Fallback: 1 PHP = 3 minutes
    minutes = coinAmount * 3;
  }
  
  // Update EEPROM
  SalesData sales = eepromMgr.read();
  sales.lifetimeIncome += coinAmount;
  sales.coinTotal += coinAmount;
  sales.customerCount++;
  eepromMgr.write(sales);
  
  // Call MikroTik Telnet
  bool mtOk = false;
  String voucher = "";
  if (strlen(sysCfg.mtIp) > 0 && strlen(sysCfg.mtUser) > 0) {
    if (mtTelnet.connect(sysCfg.mtIp, MT_TELNET_PORT, sysCfg.mtUser, sysCfg.mtPw)) {
      for (int attempt = 0; attempt < 10; attempt++) {
        String candidate = String(sysCfg.voucherPrefix) + String(random(100000, 1000000));
        if (mtTelnet.registerVoucher(candidate.c_str(), sysCfg.voucherProfile) &&
            mtTelnet.addTimeToVoucher(candidate.c_str(), minutes, minutes, 0, sysCfg.vendoName)) {
          voucher = candidate;
          mtOk = true;
          break;
        }
      }
      mtTelnet.disconnect();
    }
  }
  
  String resp = String(minutes) + "|" + (mtOk ? "1" : "0") + "|" + (mtOk ? voucher : "");
  
  // Handle MAC abuse tracking
  String mac = lockedMac;
  if (!mtOk) {
    addMacAttempt(mac);
  } else {
    clearMacAttempt(mac);
  }
  
  // Reset state
  sessionLocked = false;
  coinWindowOpen = false;
  pulseCount = 0;
  coinAmount = 0;
  lockedMac = "";
  lockedIp = "";
  digitalWrite(sysCfg.pinCoinSet, sysCfg.ledTriggerHigh ? LOW : HIGH);
  
  spiffsMgr.log("Finalized: " + resp);
  sendPlain(resp);
}

void WebServerMgr::handleRates() {
  sendCORS();
  String data = spiffsMgr.readFile(PATH_RATES);
  if (data.length() == 0) {
    data = "Piso Load|5|15|0||default#Piso Load|10|30|0||default#Piso Load|20|75|0||default#Piso Load|50|240|0||default";
  }
  sendPlain(data);
}

void WebServerMgr::handleDashboard() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  
  SalesData sales = eepromMgr.read();
  bool netOk = WiFi.status() == WL_CONNECTED;
  bool mtOk = false;
  if (strlen(sysCfg.mtIp) > 0) {
    WiFiClient test;
    mtOk = test.connect(sysCfg.mtIp, MT_TELNET_PORT, 2000);
    if (mtOk) test.stop();
  }
  
  String resp = String(millis()) + "|";
  resp += String(sales.lifetimeIncome) + "|";
  resp += String(sales.coinTotal) + "|";
  resp += String(sales.customerCount) + "|";
  resp += (netOk ? "1" : "0") + String("|");
  resp += (mtOk ? "1" : "0") + String("|");
  resp += WiFi.macAddress() + String("|");
  resp += WiFi.localIP().toString() + String("|");
  resp += "ESP8266|";
  resp += FIRMWARE_VERSION;
  sendPlain(resp);
}

void WebServerMgr::handleResetStats() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  String type = server.arg("type");
  eepromMgr.reset(type.c_str());
  sendPlain("OK");
}

void WebServerMgr::handleGetSystemConfig() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  String data = spiffsMgr.readFile(PATH_SYSTEM_CONFIG);
  if (data.length() == 0) {
    data = String(sysCfg.vendoName) + "|" + sysCfg.wifiSSID + "|" + sysCfg.wifiPW + "|" + sysCfg.mtIp + "|" + sysCfg.mtUser + "|" + sysCfg.mtPw + "|" + sysCfg.coinWaitTime + "|" + sysCfg.adminUser + "|" + sysCfg.adminPw + "|" + sysCfg.abuseCount + "|" + sysCfg.banMinutes + "|" + sysCfg.pinCoinSlot + "|" + sysCfg.pinCoinSet + "|" + sysCfg.pinReadyLed + "|" + sysCfg.pinInsertLed + "|0|" + sysCfg.pinInsertBtn + "|1|" + sysCfg.voucherPrefix + "|Welcome|" + sysCfg.setupDone + "|0|default|0|" + sysCfg.ledTriggerHigh + "|" + sysCfg.ipMode + "|" + sysCfg.localIp + "|" + sysCfg.gatewayIp + "|" + sysCfg.subnetMask + "|" + sysCfg.dnsServer;
  }
  sendPlain(data);
}

void WebServerMgr::handleSaveSystemConfig() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  String data = server.arg("data");
  if (spiffsMgr.writeFile(PATH_SYSTEM_CONFIG, data)) {
    spiffsMgr.log("Config saved");
    sendPlain("OK");
  } else {
    sendError(500, "Failed to save");
  }
}

void WebServerMgr::handleGetRates() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  String data = spiffsMgr.readFile(PATH_RATES);
  sendPlain(data);
}

void WebServerMgr::handleSaveRates() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  String data = server.arg("data");
  if (spiffsMgr.writeFile(PATH_RATES, data)) {
    spiffsMgr.log("Rates saved");
    sendPlain("OK");
  } else {
    sendError(500, "Failed to save");
  }
}

void WebServerMgr::handleGenerateVouchers() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  
  String prefix = server.arg("pfx");
  int amount = server.arg("amt").toInt();
  int qty = server.arg("qty").toInt();
  int addSales = server.arg("sales").toInt();
  
  if (prefix.length() == 0 || prefix.length() > 2) {
    sendError(400, "Invalid prefix");
    return;
  }
  if (qty < 1 || qty > 15) {
    sendError(400, "Quantity must be 1-15");
    return;
  }
  
  String output = "";
  for (int i = 0; i < qty; i++) {
    String code = prefix + String(random(100000, 999999));
    if (i > 0) output += "\n";
    output += code + " - PHP " + String(amount);
    
    // Save to SPIFFS
    String line = code + "|" + amount + "|" + millis() + "\n";
    spiffsMgr.appendFile(PATH_VOUCHERS, line);
  }
  
  if (addSales) {
    SalesData sales = eepromMgr.read();
    sales.lifetimeIncome += amount * qty;
    sales.coinTotal += amount * qty;
    eepromMgr.write(sales);
  }
  
  spiffsMgr.log("Generated " + String(qty) + " vouchers");
  sendPlain(output);
}

void WebServerMgr::handleUpdateFirmware() {
  sendCORS();
  if (!isAuthorized()) {
    handleNotAuthorized();
    return;
  }
  // Backup config before OTA
  backupSystemConfig();
  // ESP8266HTTPUpdateServer handles this automatically via /update
  sendPlain("Use /update endpoint for OTA");
}
