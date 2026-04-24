#ifndef MIKROTIK_API_H
#define MIKROTIK_API_H

#include <Arduino.h>
#include <WiFiClient.h>

class MikrotikApi {
public:
  bool connect(const char* ip, uint16_t port, const char* user, const char* pass);
  void disconnect();
  bool isConnected();
  bool createHotspotUser(const char* mac, const char* profile, uint32_t uptimeSec, const char* comment);
  bool extendHotspotUser(const char* mac, uint32_t additionalSec);
  bool removeHotspotUser(const char* mac);
  
private:
  WiFiClient client;
  bool login(const char* user, const char* pass);
  bool sendSentence(const char** words, int count);
  bool readResponse(String& out);
  bool readUntilDone();
};

extern MikrotikApi mtApi;

#endif
