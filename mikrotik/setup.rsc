# Piso WiFi MikroTik Setup Script (Telnet-based - JuanFi Compatible)
# Run this in MikroTik Terminal (New Terminal)
# Adjust values below to match your network

# ========================
# CONFIGURATION VARIABLES
# ========================
:global HOTSPOTIFACE "wlan1"
:global HOTSPOTPOOL "10.0.0.0/24"
:global HOTSPOTGATEWAY "10.0.0.1"
:global ESPIP "10.0.0.2"
:global ESPMAC "XX:XX:XX:XX:XX:XX"
:global TELNETUSER "esptelnet"
:global TELNETPASS "rekpesowifipassword"
:global VENDONAME "DormitoryWifi"
# ========================
# 1. CREATE IP POOL (Reserve 2-50 for static devices)
# ========================
/ip pool add name=hs-pool ranges=10.0.0.51-10.0.0.200

# ========================
# 2. CREATE DHCP SERVER (with gateway DNS for captive portal)
# ========================
/ip dhcp-server add name=hs-dhcp interface=$HOTSPOTIFACE address-pool=hs-pool lease-time=1h disabled=no
/ip dhcp-server network add address=10.0.0.0/24 gateway=$HOTSPOTGATEWAY dns-server=$HOTSPOTGATEWAY

# ========================
# 3. ENABLE HOTSPOT (with proper timeouts)
# ========================
/ip hotspot add name=hotspot1 interface=$HOTSPOTIFACE address-pool=hs-pool profile=default addresses-per-mac=1 idle-timeout=5m keepalive-timeout=15m session-timeout=4h
/ip hotspot profile set [ find default=yes ] html-directory=hotspot login-by=http-chap,http-pap,mac-cookie smtp-server=0.0.0.0 split-user-domain=no default-domain="" dns-name="" hotspot-address=$HOTSPOTGATEWAY idle-timeout=5m keepalive-timeout=15m session-timeout=4h

# ========================
# 4. ADD IP BINDING (ESP BYPASS)
# ========================
/ip hotspot ip-binding add mac-address=$ESPMAC address=$ESPIP type=bypassed comment="ESP8266 Controller"

# ========================
# 5. WALLED GARDEN (ALLOW PORTAL + ESP BEFORE LOGIN)
# ========================
/ip hotspot walled-garden add dst-host="*10.0.0.1*" action=allow comment="Portal IP"
/ip hotspot walled-garden add dst-host="*10.0.0.2*" action=allow comment="ESP IP"
/ip hotspot walled-garden add dst-address=10.0.0.2 action=allow comment="ESP Direct"

# ========================
# 6. ENABLE TELNET SERVICE (for ESP communication)
# ========================
/ip service enable telnet
/ip service set [find name="telnet"] port=23 address=10.0.0.2/32

# ========================
# 7. CREATE TELNET USER FOR ESP (with full permissions)
# ========================
/user add name=$TELNETUSER group=full password=$TELNETPASS comment="ESP8266 Telnet Access"

# ========================
# 8. CREATE HOTSPOT USER PROFILE (BANDWIDTH LIMITS + TIMEOUTS)
# ========================
/ip hotspot user profile add name=default shared-users=1 rate-limit=2M/2M idle-timeout=5m keepalive-timeout=15m session-timeout=4h status-autorefresh=1m

# ========================
# 9. MINIMAL LOGIN/LOGOUT SCRIPTS
# ========================
/ip hotspot profile set [find default=yes ] on-login=""
/ip hotspot profile set [find default=yes ] on-logout=""

# ========================
# 10. FIREWALL (BLOCK USERS FROM MANAGEMENT PORTS)
# ========================
/ip firewall filter add chain=input action=drop src-address=10.0.0.0/24 dst-port=8291,23 protocol=tcp comment="Block users from WinBox/Telnet"

# ========================
# DONE - VERIFY WITH:
# /ip hotspot print
# /ip hotspot ip-binding print
# /ip hotspot walled-garden print
# /user print
# /ip service print
# /ip pool print
# ========================

# ========================
# IMPORTANT NOTES:
# - ESP uses Telnet (port 23) for communication like JuanFi
# - Static IPs reserved: 10.0.0.2-50 (ESP=10.0.0.2, Gateway=10.0.0.1)
# - DHCP pool: 10.0.0.51-200 for users
# - Actual session time set by ESP based on paid promo rates
# - Maximum timeout: 4 hours (240 minutes)
# ========================
