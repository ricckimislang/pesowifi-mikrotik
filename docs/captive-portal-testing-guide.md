# Captive Portal Testing & AP Router Setup Guide

This guide helps you test the MikroTik captive portal and connect an AP router for extended WiFi coverage.

## Part 1: Testing Captive Portal with Mobile Device

### Prerequisites
- MikroTik HAP Lite configured with Piso WiFi setup
- Mobile device (phone/tablet) with WiFi
- Basic setup script executed successfully

### Step 1: Verify Hotspot is Running
```bash
# Check hotspot status
/ip hotspot print

# Should show:
# name=hotspot1 interface=wlan1 state=running

# Check if users are getting IP addresses
/ip dhcp-server lease print
```

### Step 2: Connect Mobile Device
1. **Enable WiFi** on your mobile device
2. **Find network**: "DormitoryWifi" (or your configured SSID)
3. **Connect** to the network
4. **Wait** for IP assignment (should get 10.0.0.x)

### Step 3: Test Captive Portal Redirect
1. **Open browser** on your mobile device
2. **Try to visit** any website (e.g., google.com)
3. **Should redirect** to MikroTik login page automatically

### Expected Behavior
- **Automatic redirect** to `http://10.0.0.1/login`
- **MikroTik hotspot login page** appears
- **Cannot access internet** without logging in
- **Portal shows**: Login form, status, possibly rates

### Step 4: Verify Walled Garden
Test these URLs without logging in (should work):
- `http://10.0.0.1` - Main portal
- `http://10.0.0.2` - ESP controller (if connected)

These should be blocked without login:
- `http://google.com`
- `http://facebook.com`
- Any other external site

## Part 2: Connecting AP Router to MikroTik

### Network Design
```
Internet → MikroTik HAP Lite (10.0.0.1) → AP Router → Mobile Devices
                                      ↓
                                 ESP8266 (10.0.0.2)
```

### Option 1: AP as Access Point (Recommended)
Configure your AP router as a simple access point:

#### AP Router Configuration
1. **Connect AP** to MikroTik LAN port
2. **Access AP admin** (usually 192.168.1.1 or similar)
3. **Set AP mode** to "Access Point" or "Bridge"
4. **Disable DHCP** on AP router
5. **Set SSID** to same as MikroTik (or different)
6. **Connect devices** will get IPs from MikroTik (10.0.0.x range)

#### Physical Connection
```
MikroTik LAN Port → Ethernet Cable → AP Router WAN/LAN Port
```

### Option 2: AP with Separate Subnet
If AP must have DHCP:

#### AP Router Settings
- **AP IP**: 10.0.0.3
- **AP DHCP**: 10.0.0.100-150
- **Gateway**: 10.0.0.1
- **DNS**: 10.0.0.1

#### MikroTik Additional Configuration
```bash
# Add AP to walled garden
/ip hotspot walled-garden add dst-address=10.0.0.3 action=allow comment="AP Router"

# Allow AP subnet
/ip hotspot walled-garden add dst-address=10.0.0.100-10.0.0.150 action=allow comment="AP DHCP Range"
```

## Part 3: Testing Complete Setup

### Test 1: Direct Connection to MikroTik
1. Connect mobile to "PisoWiFi" directly
2. Verify captive portal appears
3. Try to access external sites (should be blocked)

### Test 2: Connection via AP Router
1. Connect mobile to AP router's WiFi
2. Verify IP assignment (should be 10.0.0.x)
3. Test captive portal redirect
4. Verify same behavior as direct connection

### Test 3: Portal Functionality
1. **Test login** with any username/password
2. **Verify internet access** after login
3. **Check session timeout** behavior
4. **Test logout** functionality

## Part 4: Custom Portal Setup (Optional)

### Replace Default Portal
If you want custom portal pages:

#### Upload Custom Files
```bash
# Upload your portal files to hotspot directory
/tool fetch url=http://yourserver/portal.html dst-path=hotspot/portal.html
/tool fetch url=http://yourserver/login.html dst-path=hotspot/login.html
```

#### Update Hotspot Profile
```bash
/ip hotspot profile set default html-directory=hotspot
```

### Custom Portal Requirements
- **Login form** must submit to `/login`
- **POST fields**: username, password, dst (redirect URL)
- **Success page** should redirect to original destination
- **Logout link** should point to `/logout`

## Part 5: Troubleshooting Captive Portal

### Issue: No Redirect to Portal
**Symptoms**: Device connects but can browse internet freely
**Solutions**:
```bash
# Check if hotspot is enabled
/ip hotspot print

# Check walled garden rules
/ip hotspot walled-garden print

# Verify IP binding
/ip hotspot ip-binding print

# Restart hotspot
/ip hotspot disable hotspot1
/ip hotspot enable hotspot1
```

### Issue: Can't Access Portal
**Symptoms**: Browser shows error, no login page
**Solutions**:
```bash
# Check DNS settings
/ip hotspot profile print

# Verify hotspot address
/ip hotspot print

# Test direct access
# Try http://10.0.0.1 in browser
```

### Issue: AP Router Not Working
**Symptoms**: Devices connected to AP can't get IP or portal
**Solutions**:
1. **Check physical connection** between routers
2. **Verify AP DHCP is disabled** (Option 1 setup)
3. **Check AP mode** is set to Access Point/Bridge
4. **Verify AP IP doesn't conflict** with MikroTik

## Part 6: Verification Commands

### Check Current Status
```bash
# Active hotspot users
/ip hotspot active print

# DHCP leases
/ip dhcp-server lease print

# Hotspot status
/ip hotspot print detail

# Walled garden rules
/ip hotspot walled-garden print

# IP bindings
/ip hotspot ip-binding print
```

### Monitor Live Connections
```bash
# Watch new connections
/tool torch interface=wlan1

# Monitor hotspot activity
/ip hotspot active print interval=5
```

## Part 7: Production Considerations

### Security
- **Change default admin password**
- **Disable unused services**
- **Update RouterOS firmware**
- **Monitor connection logs**

### Performance
- **Monitor bandwidth usage**
- **Adjust user limits as needed**
- **Consider multiple APs for large areas**
- **Plan for peak usage times**

### Backup
```bash
# Export working configuration
/export file=working-setup

# Save to computer for recovery
```

## Quick Test Checklist

- [ ] Mobile device connects to WiFi
- [ ] Gets IP address (10.0.0.x)
- [ ] Browser redirects to login page
- [ ] External sites blocked without login
- [ ] Portal pages (10.0.0.1) accessible
- [ ] Login functionality works
- [ ] Internet access after login
- [ ] AP router extends coverage
- [ ] Same behavior through AP
- [ ] Session timeout works

---

This setup provides a complete captive portal system with extended WiFi coverage through AP routers, all using the 10.0.0.1 gateway as requested.
