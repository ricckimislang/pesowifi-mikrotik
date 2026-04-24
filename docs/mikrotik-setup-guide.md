# MikroTik HAP Lite Piso WiFi Setup Guide

This guide provides step-by-step instructions for setting up a MikroTik HAP Lite router for Piso WiFi operations with ESP8266 integration.

## Table of Contents
1. [Pre-setup Requirements](#pre-setup-requirements)
2. [Initial Router Configuration](#initial-router-configuration)
3. [Script Customization](#script-customization)
4. [MAC Address Management](#mac-address-management)
5. [Setup Execution](#setup-execution)
6. [Verification Steps](#verification-steps)
7. [Troubleshooting](#troubleshooting)
8. [ESP Integration](#esp-integration)

## Pre-setup Requirements

### Hardware Needed
- MikroTik HAP Lite router
- Ethernet cable for initial configuration
- Computer with WinBox installed
- ESP8266 device (for later integration)

### Software Needed
- [WinBox](https://mikrotik.com/download) - MikroTik configuration tool
- Text editor (for script editing)

### Network Planning
- Default network: 10.0.0.0/24
- Router IP: 10.0.0.1
- ESP IP: 10.0.0.2
- User DHCP range: 10.0.0.51-200

## Initial Router Configuration

### 1. Connect to Router
1. Connect Ethernet cable from computer to HAP Lite LAN port
2. Open WinBox
3. Click "Neighbors" tab and select your router (MAC address starts with `4C:5E:0C`)
4. Click "Connect" (default: no password, user: admin)

### 2. Basic Router Setup
```bash
# Set admin password (recommended)
# password = rekpesowifi
/user set admin password=rekpesowifi

# Configure wireless interface
/interface wireless set wlan1 mode=ap-bridge band=2ghz-b/g/n frequency=auto ssid=DormitoryWifi disabled=no

# Set router IP address
/ip address add address=10.0.0.1/24 interface=wlan1

# Enable wireless
/interface wireless enable wlan1
```

### 3. Reset to Defaults (if needed)
If router has previous configuration:
```bash
/system reset-configuration no-defaults=yes skip-backup=yes
```

## Script Customization

### 1. Open the Setup Script
Navigate to `mikrotik/setup.rsc` and edit the configuration variables:

```bash
# ========================
# CONFIGURATION VARIABLES
# ========================
:local HOTSPOTIFACE "wlan1"           # Wireless interface name
:local HOTSPOTPOOL "10.0.0.0/24"       # Network subnet
:local HOTSPOTGATEWAY "10.0.0.1"      # Router IP address
:local ESPIP "10.0.0.2"               # ESP static IP
:local ESPMAC "XX:XX:XX:XX:XX:XX"     # ESP MAC address (update later)
:local TELNETUSER "esptelnet"         # ESP telnet username
:local TELNETPASS "rekpesowifipassword" # ESP telnet password (CHANGE THIS!)
:local VENDONAME "DormitoryWifi"           # Your business name
```

### 2. Required Changes
- **`TELNETPASS`** - Change from default password
- **`VENDONAME`** - Set to your business name
- **`ESPMAC`** - Leave as placeholder for now

### 3. Optional Changes
- **`HOTSPOTIFACE`** - Only if using different interface
- **Network settings** - Only if you need different IP range

## MAC Address Management

### Finding ESP MAC Address
When you get your ESP8266:
1. Connect ESP to power and network
2. Check router's DHCP leases: `/ip dhcp-server lease print`
3. Or check ESP serial monitor output
4. Look for MAC address format: `XX:XX:XX:XX:XX:XX`

### Updating MAC Address Later

#### Method 1: Update Existing Binding
```bash
# Replace old MAC with new one
/ip hotspot ip-binding set [find comment="ESP8266 Controller"] mac-address=AA:BB:CC:DD:EE:FF
```

#### Method 2: Remove and Re-add
```bash
# Remove old binding
/ip hotspot ip-binding remove [find comment="ESP8266 Controller"]

# Add new binding
/ip hotspot ip-binding add mac-address=AA:BB:CC:DD:EE:FF address=10.0.0.2 type=bypassed comment="ESP8266 Controller"
```

#### Method 3: Run Just the Binding Command
Copy and run this single command from the setup script:
```bash
/ip hotspot ip-binding add mac-address=AA:BB:CC:DD:EE:FF address=10.0.0.2 type=bypassed comment="ESP8266 Controller"
```

### Verifying MAC Binding
```bash
# Check current bindings
/ip hotspot ip-binding print

# Look for your ESP entry
/ip hotspot ip-binding print where comment="ESP8266 Controller"
```

## Setup Execution

### 1. Run the Complete Setup Script
1. In WinBox, click **New Terminal**
2. Copy the entire content of `mikrotik/setup.rsc`
3. Paste into terminal
4. Press Enter
5. Wait for all commands to complete (may take 1-2 minutes)

### 2. What the Script Does
- Creates IP pool for DHCP
- Sets up DHCP server with gateway as DNS
- Enables hotspot with proper timeouts
- Creates ESP bypass binding (with placeholder MAC)
- Configures walled garden for portal access
- Enables Telnet service for ESP communication
- Creates ESP user with full permissions
- Sets bandwidth limits and user profiles
- Configures firewall rules for security

## Verification Steps

### 1. Check Hotspot Configuration
```bash
/ip hotspot print
```
Should show:
- Name: hotspot1
- Interface: wlan1
- Address-pool: hs-pool

### 2. Verify ESP Binding
```bash
/ip hotspot ip-binding print
```
Should show ESP binding (even with placeholder MAC)

### 3. Check Walled Garden
```bash
/ip hotspot walled-garden print
```
Should show entries for 10.0.0.1 and 10.0.0.2

### 4. Verify User Account
```bash
/user print where name=esptelnet
```
Should show your ESP telnet user

### 5. Check Services
```bash
/ip service print
```
Should show Telnet enabled and restricted to 10.0.0.2

### 6. Test Hotspot
1. Connect a device to "PisoWiFi" network
2. Open browser - should see MikroTik login page
3. Verify you can't access internet without login

## Troubleshooting

### Common Issues

#### Wireless Not Working
```bash
# Check wireless status
/interface wireless print

# Enable if disabled
/interface wireless enable wlan1

# Check configuration
/interface wireless print detail
```

#### DHCP Not Working
```bash
# Check DHCP server
/ip dhcp-server print

# Check network settings
/ip dhcp-server network print

# Restart DHCP service
/ip dhcp-server disable hs-dhcp
/ip dhcp-server enable hs-dhcp
```

#### Hotspot Not Working
```bash
# Check hotspot status
/ip hotspot print

# Check if enabled
/ip hotspot enable hotspot1

# Check profile settings
/ip hotspot profile print
```

#### Can't Access Management
```bash
# Check if you're blocked by firewall
/ip firewall filter print

# Temporarily disable firewall for testing
/ip firewall filter disable [find comment="Block users from WinBox/Telnet"]
```

### Reset Commands
If something goes wrong:
```bash
# Remove hotspot completely
/ip hotspot remove hotspot1

# Remove DHCP server
/ip dhcp-server remove hs-dhcp

# Remove IP pool
/ip pool remove hs-pool

# Start over with fresh setup
```

## ESP Integration

### 1. ESP Network Configuration
Configure your ESP8266 with these settings:
- **Static IP**: 10.0.0.2
- **Gateway**: 10.0.0.1
- **Subnet**: 255.255.255.0
- **DNS**: 10.0.0.1

### 2. ESP Telnet Connection
```cpp
// Example ESP connection settings
const char* MIKROTIK_IP = "10.0.0.1";
const int MIKROTIK_PORT = 23;
const char* TELNET_USER = "esptelnet";
const char* TELNET_PASS = "your_secure_password";
```

### 3. Test ESP Connection
1. Update ESP MAC address in router (see [MAC Address Management](#mac-address-management))
2. Upload ESP code
3. Check ESP serial monitor for connection status
4. Verify ESP can execute MikroTik commands

### 4. Common ESP Commands
The ESP will typically send commands like:
```bash
# Add user time
/ip hotspot user add name=user123 password=pass123 profile=default

# Remove user
/ip hotspot user remove [find name=user123]

# Check active users
/ip hotspot active print
```

## Security Notes

### Important Security Practices
1. **Change default passwords** - Never use "securepassword123"
2. **Update firmware** - Keep MikroTik RouterOS updated
3. **Monitor logs** - Check for unauthorized access attempts
4. **Network isolation** - Consider separating management from user traffic
5. **Regular backups** - Export configuration regularly

### Backup Configuration
```bash
# Export current configuration
/export file=backup-config

# Download via WinBox: Files → backup-config.rsc
```

## Performance Optimization

### Bandwidth Management
The script sets 2Mbps upload/download limits. To adjust:
```bash
/ip hotspot user profile set default rate-limit=1M/1M
```

### Timeout Settings
Default timeouts in script:
- **Idle timeout**: 5 minutes
- **Keepalive timeout**: 15 minutes  
- **Session timeout**: 4 hours maximum

To modify:
```bash
/ip hotspot user profile set default idle-timeout=10m keepalive-timeout=30m session-timeout=8h
```

## Final Checklist

- [ ] Router reset to defaults
- [ ] Admin password set
- [ ] Wireless configured and enabled
- [ ] Script variables customized
- [ ] Setup script executed successfully
- [ ] All verification commands pass
- [ ] Test device shows captive portal
- [ ] ESP MAC address updated (when available)
- [ ] ESP connects and can execute commands
- [ ] Configuration backed up

## Support

For issues not covered in this guide:
1. Check MikroTik documentation: https://help.mikrotik.com/
2. Review RouterOS hotspot configuration
3. Test with minimal configuration first
4. Check ESP8266 documentation for telnet implementation

---

**Note**: This setup is designed for JuanFi-compatible Piso WiFi operations using Telnet communication between ESP8266 and MikroTik router.
