# Piso WiFi MikroTik Setup Script (Telnet-based - JuanFi Compatible)
# Run this in MikroTik Terminal (New Terminal)
# Adjust values below to match your network

# ========================
# CONFIGURATION VARIABLES
# ========================
:global HOTSPOTPOOL "10.0.0.0/24"        # network subnet
:global VENDONAME "DormitoryWifi"        # your business name
:global HOTSPOTIFACE "bridge-local"      # use bridge interface when wlan1 is a bridge slave
:global HOTSPOTGATEWAY "10.0.0.1"         # Router IP address
:global ESPIP "10.0.0.2"                  # ESP static IP
:global ESPMAC "AA:BB:CC:DD:EE:FF"        # replace with actual ESP MAC address
:global TELNETUSER "esptelnet"             # ESP telnet username
:global TELNETPASS "rekpesowifipassword"  # ESP telnet password (CHANGE THIS to secure password!)

# ========================
# 1. CREATE IP POOL (Reserve 2-50 for static devices)
# ========================
/ip pool add name=hs-pool ranges=10.0.0.51-10.0.0.200

# ========================
# 2. CREATE DHCP SERVER (with gateway DNS for captive portal)
# ========================
/ip dhcp-server add name=hs-dhcp interface=bridge-local address-pool=hs-pool lease-time=1h disabled=no
/ip dhcp-server network add address=10.0.0.0/24 gateway=$HOTSPOTGATEWAY dns-server=$HOTSPOTGATEWAY

# ========================
# 3. ENABLE HOTSPOT (with proper timeouts)
# ========================
/ip hotspot add name=hotspot1 interface=$HOTSPOTIFACE address-pool=hs-pool profile=default addresses-per-mac=1
/ip hotspot profile set [find name=default] html-directory=hotspot
/ip hotspot profile set [find name=default] login-by=http-chap,http-pap,mac-cookie
/ip hotspot profile set [find name=default] smtp-server=0.0.0.0 split-user-domain=no
/ip hotspot profile set [find name=default] hotspot-address=$HOTSPOTGATEWAY

# ========================
# 4. HOTSPOT USER PROFILE (BANDWIDTH LIMITS + TIMEOUTS)
# ========================
/ip hotspot user profile set [find name=default] shared-users=1 rate-limit=2M/2M idle-timeout=5m keepalive-timeout=15m session-timeout=4h status-autorefresh=1m

# ========================
# 5. ADD IP BINDING (ESP BYPASS)
# ========================
/ip hotspot ip-binding add mac-address=$ESPMAC address=$ESPIP type=bypassed comment="ESP8266 Controller"

# ========================
# 6. WALLED GARDEN (ALLOW PORTAL + ESP BEFORE LOGIN)
# FIX: Use walled-garden ip for IP addresses, not walled-garden (which is for hostnames)
# ========================
/ip hotspot walled-garden ip add dst-address=$HOTSPOTGATEWAY action=accept comment="Portal IP"
/ip hotspot walled-garden ip add dst-address=$ESPIP action=accept comment="ESP IP"

# ========================
# 7. ENABLE TELNET SERVICE (for ESP communication only)
# ========================
/ip service enable telnet
/ip service set [find name="telnet"] port=23 address=10.0.0.2/32

# ========================
# 8. CREATE TELNET USER FOR ESP (with full permissions)
# ========================
/user add name=$TELNETUSER group=full password=$TELNETPASS comment="ESP8266 Telnet Access"

# ========================
# 9. CLEAR LOGIN/LOGOUT SCRIPTS
# ========================
/ip hotspot profile set [find name=default] on-login="" on-logout=""

# ========================
# 10. FIREWALL (BLOCK USERS FROM MANAGEMENT PORTS)
# FIX: Allow ESP Telnet FIRST, then drop rule — order matters in RouterOS firewall
# ========================
/ip firewall filter add chain=input in-interface=$HOTSPOTIFACE src-address=$ESPIP action=accept dst-port=23 protocol=tcp comment="Allow ESP Telnet access"
/ip firewall filter add chain=input in-interface=$HOTSPOTIFACE action=drop dst-port=8291,23 protocol=tcp comment="Block hotspot users from management ports"

# ========================
# DONE - VERIFY WITH:
    /ip hotspot print
    /ip hotspot ip-binding print
    /ip hotspot walled-garden ip print
    /user print
    /ip service print
    /ip pool print
========================

# ========================
# IMPORTANT NOTES:
# - ESP uses Telnet (port 23) for communication like JuanFi
# - Static IPs reserved: 10.0.0.2-50 (ESP=10.0.0.2, Gateway=10.0.0.1)
# - DHCP pool: 10.0.0.51-200 for users
# - Actual session time set by ESP based on paid promo rates
# - Maximum timeout: 4 hours (240 minutes)
# - Telnet is restricted to ESP IP only; use WinBox via ether1 for admin access
# ========================
