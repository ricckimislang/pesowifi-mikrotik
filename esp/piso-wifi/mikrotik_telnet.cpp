#include "mikrotik_telnet.h"

namespace {
static const unsigned long MT_TELNET_TIMEOUT = 5000;
static const unsigned long MT_TELNET_QUIET_MS = 120;

static void drainClient(WiFiClient& c) {
  while (c.available()) {
    (void)c.read();
  }
}

static bool responseHasError(String resp) {
  resp.toLowerCase();
  // Check for explicit error indicators in various formats
  if (resp.indexOf("failure") >= 0 || resp.indexOf("error") >= 0 || resp.indexOf("already have") >= 0 ||
      resp.indexOf("invalid") >= 0 || resp.indexOf("no such item") >= 0 || resp.indexOf("err:") >= 0 ||
      resp.indexOf("login incorrect") >= 0) {
    return true;
  }
  // Check for RouterOS error output format markers
  if (resp.indexOf("ERR") >= 0) return true;           // Explicit error from :put statements
  if (resp.indexOf(":put \"err") >= 0) return true;    // Error messages from conditionals
  if (resp.indexOf("was not executed") >= 0) return true;
  if (resp.indexOf("negative number") >= 0) return true;
  if (resp.indexOf("bad command name") >= 0) return true;
  
  // If response is only the prompt, likely a silent error
  String trimmed = resp;
  trimmed.trim();
  if (trimmed.length() <= 1) return true;
  
  return false;
}

static String sanitizeRouterOsQuoted(String s) {
  s.replace("\"", "'");
  s.replace("\r", " ");
  s.replace("\n", " ");
  return s;
}
} // namespace

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
  String buf;
  unsigned long start = millis();
  unsigned long lastByte = 0;

  // Wait for either the "Login:" prompt or an already-authenticated CLI prompt.
  while (millis() - start < MT_TELNET_TIMEOUT) {
    while (client.available()) {
      char c = client.read();
      buf += c;
      lastByte = millis();
    }

    String lower = buf;
    lower.toLowerCase();
    if (lower.indexOf("login:") >= 0 || buf.indexOf(promptChar) >= 0) {
      if (!client.available() && lastByte != 0 && (millis() - lastByte) >= MT_TELNET_QUIET_MS) break;
    }
    delay(5);
  }

  String lowerBuf = buf;
  lowerBuf.toLowerCase();

  // If we're already at the CLI prompt, consider login successful.
  if (buf.indexOf(promptChar) >= 0 && lowerBuf.indexOf("login:") < 0) {
    drainClient(client);
    return true;
  }

  if (lowerBuf.indexOf("login:") < 0) return false;

  // Send username
  drainClient(client);
  client.print(user);
  client.print("\r\n");
  delay(50);

  // Wait for password prompt
  buf = "";
  start = millis();
  lastByte = 0;
  while (millis() - start < MT_TELNET_TIMEOUT) {
    while (client.available()) {
      char c = client.read();
      buf += c;
      lastByte = millis();
    }
    String lower = buf;
    lower.toLowerCase();
    if (lower.indexOf("password:") >= 0) {
      if (!client.available() && lastByte != 0 && (millis() - lastByte) >= MT_TELNET_QUIET_MS) break;
    }
    delay(5);
  }
  String lowerPw = buf;
  lowerPw.toLowerCase();
  if (lowerPw.indexOf("password:") < 0) return false;

  // Send password
  drainClient(client);
  client.print(pass);
  client.print("\r\n");
  delay(50);

  // Check for successful login (CLI prompt reappears)
  String success;
  if (!readUntilPrompt(success)) return false;
  return !responseHasError(success);
}

bool MikrotikTelnet::sendCommand(const char* cmd) {
  if (!isConnected()) return false;
  // Avoid mixing previous output into the next response.
  drainClient(client);
  client.println(cmd);
  delay(20);
  return true;
}

String MikrotikTelnet::readResponse() {
  String out;
  (void)readUntilPrompt(out);
  return out;
}

bool MikrotikTelnet::readUntilPrompt(String& out) {
  out = "";
  unsigned long start = millis();
  unsigned long lastByte = 0;
  bool seenPrompt = false;

  while (millis() - start < MT_TELNET_TIMEOUT) {
    while (client.available()) {
      char c = client.read();
      out += c;
      lastByte = millis();
      if (c == promptChar) seenPrompt = true;
    }

    // Consume the whole prompt/output by waiting for a short quiet period after we see it.
    if (seenPrompt && !client.available() && lastByte != 0 && (millis() - lastByte) >= MT_TELNET_QUIET_MS) {
      return true;
    }

    delay(5);
  }

  return seenPrompt;
}

void MikrotikTelnet::setPromptChar(char c) {
  promptChar = c;
}

bool MikrotikTelnet::registerVoucher(const char* voucher, const char* profile) {
  String cmd = "/ip hotspot user add name=\"";
  cmd += sanitizeRouterOsQuoted(String(voucher));
  cmd += "\" limit-uptime=0 comment=0";
  if (profile && strlen(profile) > 0 && strcmp(profile, "default") != 0) {
    cmd += " profile=";
    cmd += profile;
  }

  bool ok = sendCommand(cmd.c_str());
  String resp = readResponse();
  if (!ok) return false;
  if (responseHasError(resp)) return false;
  return true;
}

bool MikrotikTelnet::addTimeToVoucher(const char* voucher, int minutes, int validity, int dataLimit, const char* comment) {
  String safeVoucher = sanitizeRouterOsQuoted(String(voucher));
  String safeTail = comment ? sanitizeRouterOsQuoted(String(comment)) : "";
  String newComment = String(validity) + "m," + String(minutes) + ",1," + safeTail;
  newComment = sanitizeRouterOsQuoted(newComment);

  String cmd =
    ":local id [/ip hotspot user find name=\"" + safeVoucher + "\"];"
    ":if ([:len $id]=0) do={:put \"ERR no user\"} else={"
      ":local lpt [/ip hotspot user get $id limit-uptime];"
      ":local nlu ($lpt + " + String(minutes) + "m);"
      "/ip hotspot user set $id limit-uptime=$nlu comment=\"" + newComment + "\";"
    "}";

  bool ok = sendCommand(cmd.c_str());
  String resp = readResponse();
  if (!ok) return false;
  if (responseHasError(resp)) return false;

  if (dataLimit > 0) {
    String dataCmd =
      ":local id [/ip hotspot user find name=\"" + safeVoucher + "\"];"
      ":if ([:len $id]=0) do={:put \"ERR no user\"} else={"
        ":local dtl [/ip hotspot user get $id limit-bytes-total];"
        ":local add (" + String(dataLimit) + "*1048576);"
        ":local tdtl ($dtl + $add);"
        ":if ($dtl=0) do={:set tdtl $add};"
        "/ip hotspot user set $id limit-bytes-total=$tdtl;"
      "}";

    ok = sendCommand(dataCmd.c_str());
    resp = readResponse();
    if (!ok) return false;
    if (responseHasError(resp)) return false;
  }

  return true;
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
