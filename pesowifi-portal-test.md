# MikroTik hAP lite Hotspot Portal Setup Commands
# For RouterOS 6.49.19 - Piso WiFi Style (No Internet Required)

## Prerequisites
- MikroTik hAP lite with RouterOS 6.49.19
- WinBox or SSH access to configure
- ESP8266/ESP32 controller (optional, for automated coin system)

## Configuration Variables (Customize These)
```bash
:global HOTSPOTNAME "PisoWiFi"
:global HOTSPOTIFACE "bridge-local"
:global HOTSPOTPOOL "10.0.0.0/24"
:global HOTSPOTGATEWAY "10.0.0.1"
:global VENDORNAME "PesoWiFi"
:global WIFISSID "PesoWiFi"
```

## Step 1: Basic Network Setup
```bash
# Reset configuration (optional - start fresh)
/system reset-configuration no-defaults=yes skip-backup=yes

# Set router identity
/system identity set name=$HOTSPOTNAME

# Configure bridge interface
/interface bridge add name=bridge-local
/interface bridge port add bridge=bridge-local interface=wlan1
/interface bridge port add bridge=bridge-local interface=ether2
/interface bridge port add bridge=bridge-local interface=ether3
/interface bridge port add bridge=bridge-local interface=ether4

# Set bridge IP
/ip address add address=10.0.0.1/24 interface=bridge-local comment="Hotspot Gateway"
```

## Step 2: Wireless Configuration
```bash
# Configure wireless for hotspot (open network)
/interface wireless set wlan1 mode=ap-bridge ssid=$WIFISSID frequency=auto band=2ghz-b/g/n \
    disabled=no default-authentication=yes hide-ssid=no wps-mode=disabled \
    wmm-support=enabled security-profile=default
```

## Step 3: IP Pool and DHCP Setup
```bash
# Create IP pool (reserve 10.0.0.2-50 for static devices)
/ip pool add name=hs-pool ranges=10.0.0.51-10.0.0.200

# Create DHCP server
/ip dhcp-server add name=hs-dhcp interface=bridge-local address-pool=hs-pool lease-time=1h disabled=no

# Add DHCP network settings
/ip dhcp-server network add address=$HOTSPOTPOOL gateway=$HOTSPOTGATEWAY \
    dns-server=$HOTSPOTGATEWAY domain=$VENDORNAME comment="Hotspot Network"
```

## Step 4: Hotspot Setup
```bash
# Create hotspot
/ip hotspot add name=hotspot1 interface=bridge-local address-pool=hs-pool profile=default \
    addresses-per-mac=1 idle-timeout=5m keepalive-timeout=15m

# Configure hotspot profile
/ip hotspot profile set [find name=default] html-directory=hotspot \
    login-by=http-chap,http-pap,mac-cookie smtp-server=0.0.0.0 \
    split-user-domain=no hotspot-address=$HOTSPOTGATEWAY

# Set user profile (time and bandwidth limits)
/ip hotspot user profile set [find name=default] session-timeout=1h \
    idle-timeout=15m keepalive-timeout=30m shared-users=1 \
    rate-limit=1M/1M status-autorefresh=1m
```

## Step 5: Walled Garden (Allow portal access without login)
```bash
# Allow access to portal and local services
/ip hotspot walled-garden ip add dst-address=$HOTSPOTGATEWAY action=accept comment="Portal IP"

# Common captive portal detection URLs
/ip hotspot walled-garden add dst-host="captive.apple.com" comment="Apple CNA"
/ip hotspot walled-garden add dst-host="connectivitycheck.gstatic.com" comment="Android CNA"
/ip hotspot walled-garden add dst-host="clients3.google.com" comment="Chrome CNA"
/ip hotspot walled-garden add dst-host="msftconnecttest.com" comment="Windows CNA"
```

## Step 6: Custom Portal Files (Optional)
```bash
# Create custom login page (upload via WinBox Files)
# File: hotspot/login.html
# File: hotspot/status.html
# File: hotspot/logout.html
# File: hotspot/alogin.html (after login)
# File: hotspot/rlogin.html (redirect login)

# Or use built-in templates
/ip hotspot profile set [find name=default] html-directory=flash/hotspot
```

## Step 7: Test Users Setup
```bash
# Create test users with different time limits
/ip hotspot user add name=15min password=15min profile=default comment="15 minutes"
/ip hotspot user add name=30min password=30min profile=default comment="30 minutes"
/ip hotspot user add name=1hour password=1hour profile=default comment="1 hour"

# Create voucher-style users (numeric usernames)
/ip hotspot user add name=001 password=001 profile=default comment="Voucher 001"
/ip hotspot user add name=002 password=002 profile=default comment="Voucher 002"
/ip hotspot user add name=003 password=003 profile=default comment="Voucher 003"
```

## Step 8: Firewall Rules (Security)
```bash
# Block hotspot users from management ports
/ip firewall filter add chain=input in-interface=bridge-local \
    action=drop dst-port=8291,22,23,80,443 protocol=tcp \
    comment="Block hotspot users from management"

# Allow DNS for hotspot users
/ip firewall filter add chain=forward in-interface=bridge-local \
    action=accept protocol=udp dst-port=53 comment="Allow DNS"

# Block other traffic (no internet)
/ip firewall filter add chain=forward in-interface=bridge-local \
    action=drop comment="Block internet access"
```

## Step 9: ESP8266 Integration Setup (Optional)
```bash
# Reserve IP for ESP controller
/ip address add address=10.0.0.2/24 interface=bridge-local comment="ESP Controller"

# ESP bypass (no login required)
/ip hotspot ip-binding add mac-address=AA:BB:CC:DD:EE:FF address=10.0.0.2 \
    type=bypassed comment="ESP8266 Controller"

# Enable Telnet for ESP communication
/ip service enable telnet
/ip service set [find name="telnet"] port=23 address=10.0.0.2/32

# Create ESP user
/user add name=esptelnet group=full password=esp123password comment="ESP Telnet Access"

# Allow ESP Telnet in firewall
/ip firewall filter add chain=input in-interface=bridge-local src-address=10.0.0.2 \
    action=accept dst-port=23 protocol=tcp comment="Allow ESP Telnet"
```

## Step 10: Verification Commands
```bash
# Check hotspot status
/ip hotspot print
/ip hotspot active print
/ip hotspot user print

# Check DHCP
/ip dhcp-server print
/ip dhcp-server lease print

# Check bindings and walled garden
/ip hotspot ip-binding print
/ip hotspot walled-garden print
/ip hotspot walled-garden ip print

# Test connectivity
/ping 10.0.0.1
/ping 8.8.8.8 (should fail - no internet)
```

## Complete Setup Script (Copy-Paste Ready)
```bash
# PISO WIFI COMPLETE SETUP - CORRECTED VERSION
# Variables
:global HOTSPOTGATEWAY "10.0.0.1"
:global WIFISSID "DormitoryWifi"

# Wireless
/interface wireless set wlan1 mode=ap-bridge ssid=$WIFISSID frequency=auto band=2ghz-b/g/n disabled=no

# IP on wlan1 directly (NOT bridge)
/ip address add address=10.0.0.1/24 interface=wlan1

# DHCP
/ip pool add name=hs-pool ranges=10.0.0.51-10.0.0.200
/ip dhcp-server add name=hs-dhcp interface=wlan1 address-pool=hs-pool lease-time=1h disabled=no
/ip dhcp-server network add address=10.0.0.0/24 gateway=10.0.0.1 dns-server=10.0.0.1

# DNS
/ip dns set allow-remote-requests=yes servers=8.8.8.8,1.1.1.1

# Hotspot
/ip hotspot add name=hotspot1 interface=wlan1 address-pool=hs-pool profile=default addresses-per-mac=1 disabled=no

# Hotspot profile
/ip hotspot profile set [find name=default] html-directory=hotspot login-by=http-chap,http-pap,mac-cookie smtp-server=0.0.0.0 split-user-domain=no hotspot-address=10.0.0.1 dns-name=hotspot.local

# User profile
/ip hotspot user profile set [find name=default] session-timeout=1h idle-timeout=15m keepalive-timeout=30m shared-users=1 rate-limit=1M/1M status-autorefresh=1m

# Walled garden
/ip hotspot walled-garden ip add dst-address=10.0.0.1 action=accept comment="Portal IP"
/ip hotspot walled-garden add dst-host=captive.apple.com comment="Apple CNA"
/ip hotspot walled-garden add dst-host=connectivitycheck.gstatic.com comment="Android CNA"
/ip hotspot walled-garden add dst-host=clients3.google.com comment="Chrome CNA"
/ip hotspot walled-garden add dst-host=msftconnecttest.com comment="Windows CNA"

# DNS wildcard (forces redirect without internet)
/ip dns static add name="*" address=10.0.0.1 ttl=30s

# Test user
/ip hotspot user add name=test password=test profile=default

# NAT (for when ISP is connected)
/ip dhcp-client add interface=ether1 disabled=no add-default-route=yes
/ip firewall nat add chain=srcnat out-interface=ether1 action=masquerade comment="NAT internet"

# Firewall (safe version - does NOT block port 80)
/ip firewall filter add chain=input in-interface=wlan1 action=drop dst-port=8291,23 protocol=tcp comment="Block management ports"

# Done!
```

## Testing the Portal
1. Connect device to "DormitoryWifi" network
2. Open browser - should redirect to login page
3. Login with username: test, password: test
4. Verify 1-hour access with 1Mbps speed limit
5. After timeout, should redirect back to login page

## Troubleshooting
```bash
# Reset hotspot if needed
/ip hotspot disable hotspot1
/ip hotspot enable hotspot1

# Check logs
/log print

# Monitor active users
/ip hotspot active print

# Clear all users
/ip hotspot user remove [find]
```

## Notes
- No internet connection required for basic portal functionality
- Portal pages are served locally from MikroTik
- Adjust time limits and bandwidth as needed
- For coin system integration, use ESP8266 with Telnet commands
- Backup configuration after setup: /system backup save name=pesowifi-setup