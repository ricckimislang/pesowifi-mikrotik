#ifndef MIKROTIK_TELNET_H
#define MIKROTIK_TELNET_H

#include <Arduino.h>
#include <WiFiClient.h>

class MikrotikTelnet {
public:
  bool connect(const char* ip, uint16_t port, const char* user, const char* pass);
  void disconnect();
  bool isConnected();
  
  // Voucher-based session management (like JuanFi)
  bool registerVoucher(const char* voucher, const char* profile);
  bool addTimeToVoucher(const char* voucher, int minutes, int validity, int dataLimit, const char* comment);
  bool generateVoucher(const char* voucher, int minutes, const char* profile);
  
  // Helper
  bool sendCommand(const char* cmd);
  String readResponse();
  
private:
  WiFiClient client;
  bool loggedIn = false;
  char promptChar = '>';
  
  bool login(const char* user, const char* pass);
  bool readUntilPrompt(String& out);
  void setPromptChar(char c);
};

extern MikrotikTelnet mtTelnet;

#endif
