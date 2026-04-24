#include "mikrotik_api.h"

MikrotikApi mtApi;

bool MikrotikApi::connect(const char* ip, uint16_t port, const char* user, const char* pass) {
  if (client.connected()) client.stop();
  if (!client.connect(ip, port)) return false;
  if (!login(user, pass)) {
    client.stop();
    return false;
  }
  return true;
}

void MikrotikApi::disconnect() {
  if (client.connected()) {
    client.println("/quit");
    client.stop();
  }
}

bool MikrotikApi::isConnected() {
  return client.connected();
}

bool MikrotikApi::login(const char* user, const char* pass) {
  // Read login greeting
  String greeting;
  if (!readResponse(greeting)) return false;
  
  // Send login command
  client.print("/login\r\n");
  client.print("=name="); client.print(user); client.print("\r\n");
  client.print("=password="); client.print(pass); client.print("\r\n");
  client.print("\r\n");
  
  String response;
  if (!readResponse(response)) return false;
  return response.indexOf("!done") >= 0;
}

bool MikrotikApi::createHotspotUser(const char* mac, const char* profile, uint32_t uptimeSec, const char* comment) {
  if (!isConnected()) return false;
  
  String username = String("coin_") + String(mac);
  username.replace(":", "");
  
  client.print("/ip/hotspot/user/add\r\n");
  client.print("=name="); client.print(username); client.print("\r\n");
  client.print("=mac-address="); client.print(mac); client.print("\r\n");
  client.print("=profile="); client.print(profile); client.print("\r\n");
  client.print("=limit-uptime="); client.print(uptimeSec); client.print("\r\n");
  client.print("=comment="); client.print(comment); client.print("\r\n");
  client.print("\r\n");
  
  return readUntilDone();
}

bool MikrotikApi::extendHotspotUser(const char* mac, uint32_t additionalSec) {
  if (!isConnected()) return false;
  
  String username = String("coin_") + String(mac);
  username.replace(":", "");
  
  // Find existing user and extend
  client.print("/ip/hotspot/user/set\r\n");
  client.print("=.id="); client.print(username); client.print("\r\n");
  client.print("=limit-uptime="); client.print(additionalSec); client.print("\r\n");
  client.print("\r\n");
  
  return readUntilDone();
}

bool MikrotikApi::removeHotspotUser(const char* mac) {
  if (!isConnected()) return false;
  
  String username = String("coin_") + String(mac);
  username.replace(":", "");
  
  client.print("/ip/hotspot/user/remove\r\n");
  client.print("=.id="); client.print(username); client.print("\r\n");
  client.print("\r\n");
  
  return readUntilDone();
}

bool MikrotikApi::sendSentence(const char** words, int count) {
  if (!isConnected()) return false;
  for (int i = 0; i < count; i++) {
    client.print(words[i]);
    client.print("\r\n");
  }
  client.print("\r\n");
  return true;
}

bool MikrotikApi::readResponse(String& out) {
  out = "";
  unsigned long start = millis();
  while (millis() - start < MT_API_TIMEOUT) {
    while (client.available()) {
      char c = client.read();
      out += c;
      if (out.indexOf("!done") >= 0 || out.indexOf("!trap") >= 0) {
        return true;
      }
    }
    delay(10);
  }
  return false;
}

bool MikrotikApi::readUntilDone() {
  String response;
  unsigned long start = millis();
  while (millis() - start < MT_API_TIMEOUT) {
    while (client.available()) {
      char c = client.read();
      response += c;
    }
    if (response.indexOf("!done") >= 0) return true;
    if (response.indexOf("!trap") >= 0) return false;
    delay(10);
  }
  return false;
}
