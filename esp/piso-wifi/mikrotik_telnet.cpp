#include "mikrotik_telnet.h"

MikrotikTelnet mtTelnet;

bool MikrotikTelnet::connect(const char* ip, uint16_t port, const char* user, const char* pass) {
  if (client.connected()) client.stop();
  if (!client.connect(ip, port)) return false;
  setPromptChar('>');
  loggedIn = login(user, pass);
  if (!loggedIn) {
    client.stop();
    return false;
  }
  return true;
}

void MikrotikTelnet::disconnect() {
  if (client.connected()) {
    client.println("/quit");
    delay(100);
    client.stop();
  }
  loggedIn = false;
}

bool MikrotikTelnet::isConnected() {
  return client.connected() && loggedIn;
}

bool MikrotikTelnet::login(const char* user, const char* pass) {
  // Wait for initial prompt
  String greeting;
  if (!readUntilPrompt(greeting)) return false;
  
  // Send login
  client.print(user);
  client.print("\r\n");
  delay(100);
  
  // Wait for password prompt
  String pwPrompt;
  if (!readUntilPrompt(pwPrompt)) return false;
  
  // Send password
  client.print(pass);
  client.print("\r\n");
  delay(100);
  
  // Check for successful login (prompt reappears)
  String success;
  return readUntilPrompt(success);
}

bool MikrotikTelnet::sendCommand(const char* cmd) {
  if (!isConnected()) return false;
  client.println(cmd);
  delay(50);
  return true;
}

String MikrotikTelnet::readResponse() {
  String out;
  unsigned long start = millis();
  while (millis() - start < 5000) {
    while (client.available()) {
      char c = client.read();
      out += c;
    }
    if (out.indexOf(promptChar) >= 0) break;
    delay(10);
  }
  return out;
}

bool MikrotikTelnet::readUntilPrompt(String& out) {
  out = "";
  unsigned long start = millis();
  while (millis() - start < 3000) {
    while (client.available()) {
      char c = client.read();
      out += c;
    }
    if (out.indexOf(promptChar) >= 0) return true;
    delay(10);
  }
  return false;
}

void MikrotikTelnet::setPromptChar(char c) {
  promptChar = c;
}

bool MikrotikTelnet::registerVoucher(const char* voucher, const char* profile) {
  String cmd = "/ip hotspot user add name=";
  cmd += voucher;
  cmd += " limit-uptime=0 comment=0";
  if (profile && strlen(profile) > 0 && strcmp(profile, "default") != 0) {
    cmd += " profile=";
    cmd += profile;
  }
  if (!sendCommand(cmd.c_str())) return false;
  readResponse(); // consume response
  return true;
}

bool MikrotikTelnet::addTimeToVoucher(const char* voucher, int minutes, int validity, int dataLimit, const char* comment) {
  // Read current limit-uptime
  String getCmd = ":global lpt; :global nlu; :set lpt [/ip hotspot user get ";
  getCmd += voucher;
  getCmd += " limit-uptime];";
  if (!sendCommand(getCmd.c_str())) return false;
  readResponse();

  // Calculate new uptime
  String setCmd = ":set nlu [($lpt+";
  setCmd += String(minutes);
  setCmd += "m)]; /ip hotspot user set limit-uptime=$nlu comment=\"";
  setCmd += String(validity);
  setCmd += "m,";
  setCmd += String(minutes);
  setCmd += ",1,";
  if (comment && strlen(comment) > 0) {
    setCmd += comment;
  }
  setCmd += "\" ";
  if (dataLimit > 0) {
    // Handle data limit
    String dataCmd = ":global tdtl; :global dtl [/ip hotspot user get ";
    dataCmd += voucher;
    dataCmd += " limit-bytes-total]; :if ($dtl>0) do={ :set tdtl [(dtl+";
    dataCmd += String(dataLimit);
    dataCmd += " *1048576)] } else { :set tdtl [(";
    dataCmd += String(dataLimit);
    dataCmd += " *1048576)] }; /ip hotspot user set limit-bytes-total=$tdtl ";
    dataCmd += voucher;
    sendCommand(dataCmd.c_str());
    readResponse();
  }
  setCmd += voucher;
  return sendCommand(setCmd.c_str());
}

bool MikrotikTelnet::generateVoucher(const char* voucher, int minutes, const char* profile) {
  String cmd = "/ip hotspot user add name=";
  cmd += voucher;
  cmd += " limit-uptime=";
  cmd += String(minutes * 60);
  cmd += " comment=\"generated\"";
  if (profile && strlen(profile) > 0 && strcmp(profile, "default") != 0) {
    cmd += " profile=";
    cmd += profile;
  }
  if (!sendCommand(cmd.c_str())) return false;
  readResponse();
  return true;
}
