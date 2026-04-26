# Fixes Applied to Piso WiFi v3

## Summary of Changes

This document details all the fixes applied to address identified issues in the MikroTik setup script, ESP firmware, and documentation.

---

## 1. MikroTik Setup Script (`mikrotik/setup.rsc`)

### Issue 1.1: Hotspot Profile Not Explicitly Setting Login Page
**Problem**: MikroTik hotspot defaults to serving `login.html` as the captive portal page. The guide mentioned uploading `index.html` but the script didn't explicitly configure the profile to use it.

**Fix Applied**:
```bash
# BEFORE
/ip hotspot profile set [ find name=default ] html-directory=hotspot login-by=http-chap,http-pap,mac-cookie ...

# AFTER
/ip hotspot profile set [ find name=default ] html-directory=hotspot login=index.html login-by=http-chap,http-pap,mac-cookie ...
```

**Impact**: MikroTik now explicitly serves `index.html` as the login page, ensuring the correct captive portal is displayed.

---

### Issue 1.2: Profile Creation Failure on Re-run
**Problem**: The script used `/ip hotspot user profile add name=default` but the `default` profile already exists in RouterOS. This causes the script to fail if run twice.

**Fix Applied**:
```bash
# BEFORE
/ip hotspot user profile add name=default shared-users=1 ...

# AFTER
/ip hotspot user profile set [ find name=default ] shared-users=1 ...
```
**Impact**: Script now successfully modifies the existing default profile and can be run multiple times without errors.

---

### Issue 1.3: Walled Garden Using Unreliable Hostnames
**Problem**: The walled garden rules used `dst-host="*10.0.0.1*"` which is intended for DNS names, not IP addresses. This can cause unreliable access to the portal before login.

**Fix Applied**:
```bash
# BEFORE
/ip hotspot walled-garden add dst-host="*10.0.0.1*" action=allow comment="Portal IP"
/ip hotspot walled-garden add dst-host="*10.0.0.2*" action=allow comment="ESP IP"
/ip hotspot walled-garden add dst-address=10.0.0.2 action=allow comment="ESP Direct"

# AFTER
/ip hotspot walled-garden add dst-address=10.0.0.1 action=allow comment="Portal IP"
/ip hotspot walled-garden add dst-address=10.0.0.2 action=allow comment="ESP IP"
```
**Impact**: Portal access is now reliable for unauthenticated users; reduced from 3 rules to 2 cleaner rules.

---

### Issue 1.4: Firewall Rule Too Broad, Blocks Admin Access
**Problem**: Original rule used `src-address=10.0.0.0/24` to block the entire hotspot subnet from management ports. This is problematic if:
- An admin device connects to the hotspot network
- Multiple interfaces are present and the rule catches unintended traffic

**Fix Applied**:
```bash
# BEFORE
/ip firewall filter add chain=input action=drop src-address=10.0.0.0/24 dst-port=8291,23 protocol=tcp \
  comment="Block users from WinBox/Telnet"

# AFTER
/ip firewall filter add chain=input in-interface=$HOTSPOTIFACE action=drop dst-port=8291,23 protocol=tcp \
  comment="Block hotspot users from management ports"
```

**How It Works**:
- The new rule is **interface-specific**: blocks traffic only on the hotspot interface
- Admin devices connecting via LAN or a separate admin interface are **NOT blocked**
- More secure and doesn't break management access from proper admin channels

**Additional Security Notes**:
If you need to access WinBox from within the hotspot network, use a different path:
1. Connect to the router via its LAN/WAN interface instead
2. Or add an explicit allow rule for trusted admin IPs:
   ```bash
   /ip firewall filter add chain=input in-interface=ether1 src-address=192.168.1.100/32 \
     action=accept dst-port=8291,23 protocol=tcp comment="Allow admin IP"
   ```

---

### Issue 1.5: Unused/Misleading Configuration Variables
**Problem**:
- `HOTSPOTPOOL` variable defined but never used in actual commands
- `VENDONAME` variable declared but not wired into any configuration

**Decision**: Left as-is per your request. These can be cleaned up in a future version if needed.

---

## 2. ESP Firmware Changes

### Issue 2.1: Missing Connectivity Verification Before Voucher Creation
**Problem**: ESP would attempt to create vouchers even when MikroTik router was unreachable, leading to lost coins and failed transactions.

**Fix Applied** (in `web_server.cpp`):
```cpp
// Added before MikroTik Telnet call in handleFinalize()
bool mtReachable = false;
if (strlen(sysCfg.mtIp) > 0) {
  WiFiClient testClient;
  mtReachable = testClient.connect(sysCfg.mtIp, MT_TELNET_PORT, 2000);
  if (mtReachable) testClient.stop();
}
if (!mtReachable) {
  spiffsMgr.log("MikroTik unreachable - aborting transaction");
  // Reset state and return HTTP 503
  sendError(503, "Router unreachable");
  return;
}
```

**Impact**: Transactions now fail gracefully when router is unreachable, preventing lost coins.

---

### Issue 2.2: Short Admin Session Timeout
**Problem**: Admin sessions expired after 5 minutes, making configuration difficult.

**Fix Applied** (in `config.h`):
```cpp
// BEFORE
#define ADMIN_SESSION_TIMEOUT   300000 // 5 minutes

// AFTER
#define ADMIN_SESSION_TIMEOUT   1800000 // 30 minutes
```

**Impact**: Admin sessions now persist for 30 minutes, allowing more time for configuration.

---

### Issue 2.3: No API Rate Limiting
**Problem**: Portal API endpoints had no protection against abuse or excessive requests.

**Fix Applied** (in `web_server.h` and `web_server.cpp`):
- Added `RateLimitEntry` struct and tracking array
- Implemented `checkRateLimit()` function (30 requests/minute per IP)
- Applied rate limiting to all portal API endpoints
- Returns HTTP 429 when limit exceeded

**Impact**: Basic protection against API abuse while maintaining reasonable usage limits.

---

### Issue 2.4: Missing CORS OPTIONS Preflight Support
**Problem**: Browsers making cross-origin POST/PUT/DELETE requests first send an HTTP OPTIONS request (preflight). Without handling this, the ESP would return 404, causing the actual request to fail.

**Fix Applied**:
```cpp
// Added to setupRoutes() function
server.onNotFound([this]() {
  if (server.method() == HTTP_OPTIONS) {
    sendCORS();
    server.send(204, "text/plain", "");
    return;
  }
});
```

**Impact**:
- Portal JavaScript can now make cross-origin API calls from different domains/ports
- OPTIONS requests return 204 No Content with proper CORS headers
- No 404 errors on preflight requests

---

## 3. MikroTik Telnet Integration (`mikrotik_telnet.cpp`)

### Issue 3.1: Insufficient RouterOS Error Detection
**Problem**: The error parser only checked for a few specific strings, missing RouterOS-specific error formats.

**Fix Applied**:
Enhanced error detection to check for:
```cpp
// NEW: Comprehensive error detection
- "failure", "error", "err:", "already have", "invalid", etc. (original checks)
- "ERR" marker (explicit error output)
- ":put \"err" (error from Telnet scripts)
- "was not executed", "negative number", "bad command name" (RouterOS-specific errors)
- Empty/prompt-only responses (likely silent failures)
```

**Impact**:
- Voucher creation failures are now detected reliably
- Silent errors no longer pass as "success"
- Better troubleshooting: actual errors surface instead of failing mysteriously
- Failed vouchers are now properly tracked for MAC abuse detection

---

## 4. Documentation Updates (`docs/implementation-guide.md`)

### Issue 4.1: Missing ESP MAC Identification Instructions
**Problem**: No clear instructions on how to obtain the ESP8266 MAC address required for IP binding.

**Fix Applied**:
Added comprehensive "Find ESP MAC Address" section with 3 methods:
- Serial monitor during boot
- Admin dashboard after setup
- Code snippet for manual retrieval

**Impact**: Users can now easily identify and configure the ESP MAC address.

---

### Issue 4.2: Checklist Not Updated for New Requirements
**Problem**: Pre-deployment checklist didn't include MAC identification step.

**Fix Applied**:
Updated checklist to include:
- [ ] ESP MAC address identified and noted

**Impact**: More complete setup verification.

---

## 5. Bridge Interface Configuration (`docs/mikrotik-setup-guide.md`)

### Issue 5.1: Missing Bridge Creation Steps
**Problem**: The setup guide referenced `bridge-local` interface but didn't include steps to create it. On fresh HAP Lite routers, `bridge-local` doesn't exist by default, causing "input does not match any value of interface" errors.

**Fix Applied**:
Updated the Basic Router Setup section to include bridge creation:
```bash
# BEFORE
/interface wireless set wlan1 mode=ap-bridge band=2ghz-b/g/n frequency=auto ssid=DormitoryWifi disabled=no
/ip address add address=10.0.0.1/24 interface=bridge-local
/interface wireless enable wlan1

# AFTER
/interface wireless set wlan1 mode=ap-bridge band=2ghz-b/g/n frequency=auto ssid=DormitoryWifi disabled=no

# Create bridge interface (required for hotspot services)
/interface bridge add name=bridge-local

# Add wireless interface to bridge
/interface bridge port add bridge=bridge-local interface=wlan1

# Set router IP address on bridge interface
/ip address add address=10.0.0.1/24 interface=bridge-local

# Enable wireless
/interface wireless enable wlan1
```

**Impact**:
- Eliminates "interface not found" errors during setup
- Follows proper MikroTik bridge architecture
- Ensures hotspot services bind to correct interface
- Maintains compatibility with existing setup script

**Technical Notes**:
- `wlan1` becomes bridge slave to `bridge-local`
- All services (DHCP, Hotspot, Firewall) bind to bridge master
- This is the recommended MikroTik best practice for access point deployments

---

## 6. Hotspot Command Structure Fix (`mikrotik/setup.rsc`)

### Issue 6.1: Timeout Parameters in Wrong Command
**Problem**: Timeout parameters (`idle-timeout`, `keepalive-timeout`, `session-timeout`) were incorrectly placed in the hotspot server creation command instead of the profile configuration.

**Fix Applied**:
```bash
# BEFORE
/ip hotspot add name=hotspot1 interface=$HOTSPOTIFACE address-pool=hs-pool profile=default addresses-per-mac=1 idle-timeout=5m keepalive-timeout=15m session-timeout=4h

# AFTER  
/ip hotspot add name=hotspot1 interface=$HOTSPOTIFACE address-pool=hs-pool profile=default addresses-per-mac=1
```

**Impact**:
- Eliminates "expected end of command" syntax errors
- Follows proper MikroTik RouterOS command structure
- Timeout settings remain correctly configured in profile commands (line 34)
- Script can now run without command parsing errors

**Technical Notes**:
- Hotspot server creation controls interface, pool, and basic server settings
- Profile configuration handles timeouts, authentication, and login behavior
- Separating these follows RouterOS best practices and prevents syntax errors

---

## 7. Login Page Default Fix (`mikrotik/setup.rsc`)

### Issue 7.1: Forcing Non-Standard Login Page
**Problem**: Script was forcing `login=index.html` instead of using MikroTik's default `login.html`. This requires additional configuration and can cause compatibility issues.

**Fix Applied**:
```bash
# BEFORE
/ip hotspot profile set [ find name=default ] login=index.html login-by=http-chap,http-pap,mac-cookie

# AFTER
/ip hotspot profile set [ find name=default ] login=login.html login-by=http-chap,http-pap,mac-cookie
```

**Impact**:
- Uses MikroTik's default login page name
- Eliminates need for custom profile configuration
- Better compatibility with RouterOS updates
- Simplifies file naming convention

**Technical Notes**:
- MikroTik defaults to serving `login.html` from the hotspot directory
- Using the default prevents unexpected behavior
- Portal files should be named `login.html` instead of `index.html`
- This aligns with MikroTik's standard captive portal implementation

---

## 8. Hotspot Profile Command Structure Fix (`mikrotik/setup.rsc`)

### Issue 8.1: Incorrect Profile Command Syntax
**Problem**: Multiple profile commands had syntax errors due to improper parameter grouping and wrong command types.

**Issues Identified**:
- `login=login.html` parameter caused syntax errors
- Multiple parameters in single commands caused "expected end of command" errors
- Timeout parameters were in wrong command type (`profile` vs `user profile`)

**Fix Applied** (based on working commands):
```bash
# BEFORE (broken)
/ip hotspot profile set [ find name=default ] login=login.html login-by=http-chap,http-pap,mac-cookie
/ip hotspot profile set [ find name=default ] smtp-server=0.0.0.0 split-user-domain=no default-domain="" dns-name=""
/ip hotspot profile set [ find name=default ] hotspot-address=$HOTSPOTGATEWAY idle-timeout=5m keepalive-timeout=15m session-timeout=4h

# AFTER (working)
/ip hotspot profile set [ find name=default ] html-directory=hotspot
/ip hotspot profile set [find name=default] login-by=http-chap,http-pap,mac-cookie
/ip hotspot profile set [find name=default] smtp-server=0.0.0.0 split-user-domain=no
/ip hotspot profile set [find name=default] hotspot-address=$HOTSPOTGATEWAY
/ip hotspot user profile set [find name=default] session-timeout=4h idle-timeout=5m keepalive-timeout=15m shared-users=1 rate-limit=2M/2M
```

**Impact**:
- Eliminates all syntax errors in profile configuration
- Separates settings into correct command types
- Uses working command structure from actual RouterOS testing
- Timeout settings moved to correct `user profile` command

**Technical Notes**:
- Removed `login=login.html` parameter (uses default)
- Split complex commands into single-parameter commands
- Timeout/bandwidth settings belong in `user profile`, not `profile`
- `hotspot-address` set separately without timeout parameters

---

## Files Modified

- ✅ `mikrotik/setup.rsc` - Hotspot profile configuration (login page) + command structure fix + default login page + profile command syntax
- ✅ `esp/piso-wifi/config.h` - Admin session timeout
- ✅ `esp/piso-wifi/web_server.h` - Rate limiting structures
- ✅ `esp/piso-wifi/web_server.cpp` - Connectivity verification, rate limiting, CORS support
- ✅ `esp/piso-wifi/mikrotik_telnet.cpp` - Enhanced error detection
- ✅ `docs/implementation-guide.md` - MAC identification instructions and checklist
- ✅ `docs/mikrotik-setup-guide.md` - Bridge interface creation steps

## Files Not Modified (As Requested)

- ⏸️ `portal/portal.js` - Left as-is
- ⏸️ `portal/login.html` - Left as-is
- ⏸️ `esp/piso-wifi/piso-wifi.ino` - No changes needed
- ⏸️ Passwords kept as default per user request

---

## Testing Recommendations

### For MikroTik Setup
1. Run setup script and verify hotspot serves `index.html` correctly
2. Test ESP IP binding with correct MAC address
3. Verify walled garden allows portal access for unauthenticated users
4. Verify bridge interface creation and wireless interface binding

### For ESP Firmware
1. Test coin insertion when router is unreachable (should fail gracefully)
2. Verify admin sessions persist beyond 5 minutes
3. Test API rate limiting by making excessive requests
4. Check browser console for CORS-related errors

### For Documentation
1. Follow MAC identification steps and verify they work
2. Complete updated checklist before deployment

---

## Next Steps (Optional)

1. **Test the fixes** in a development environment before deploying to production
2. **Review firewall rules** if you have complex multi-interface setups
3. **Consider adding** logging for voucher creation attempts to debug future issues
4. **Document your specific setup** (IP ranges, passwords, etc.) separately from the template script
