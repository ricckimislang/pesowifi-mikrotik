# Piso WiFi MikroTik Setup Script
# Run this in MikroTik Terminal (New Terminal)
# Adjust values below to match your network

# ========================
# CONFIGURATION VARIABLES
# ========================
:local HOTSPOT_IFACE "wlan1"
:local HOTSPOT_POOL "10.0.0.0/24"
:local HOTSPOT_GATEWAY "10.0.0.1"
:local ESP_IP "10.0.0.2"
:local ESP_MAC "XX:XX:XX:XX:XX:XX"
:local API_USER "espapi"
:local API_PASS "securepassword123"
:local VENDO_NAME "PisoWiFi"

# ========================
# 1. CREATE IP POOL
# ========================
/ip pool add name=hs-pool ranges=10.0.0.10-10.0.0.250

# ========================
# 2. CREATE DHCP SERVER
# ========================
/ip dhcp-server add name=hs-dhcp interface=$HOTSPOT_IFACE address-pool=hs-pool lease-time=1h disabled=no
/ip dhcp-server network add address=10.0.0.0/24 gateway=$HOTSPOT_GATEWAY dns-server=8.8.8.8,8.8.4.4

# ========================
# 3. ENABLE HOTSPOT
# ========================
/ip hotspot add name=hotspot1 interface=$HOTSPOT_IFACE address-pool=hs-pool profile=default addresses-per-mac=1 idle-timeout=5m keepalive-timeout=15m
/ip hotspot profile set [ find default=yes ] html-directory=hotspot login-by=http-chap,mac-cookie smtp-server=0.0.0.0 split-user-domain=no default-domain="" dns-name="" hotspot-address=$HOTSPOT_GATEWAY

# ========================
# 4. ADD IP BINDING (ESP BYPASS)
# ========================
/ip hotspot ip-binding add mac-address=$ESP_MAC address=$ESP_IP type=bypassed comment="ESP8266 Controller"

# ========================
# 5. WALLED GARDEN (ALLOW PORTAL + ESP BEFORE LOGIN)
# ========================
/ip hotspot walled-garden add dst-host="*10.0.0.1*" action=allow comment="Portal IP"
/ip hotspot walled-garden add dst-host="*10.0.0.2*" action=allow comment="ESP IP"
/ip hotspot walled-garden add dst-address=10.0.0.2 action=allow comment="ESP Direct"

# ========================
# 6. ENABLE API SERVICE
# ========================
/ip service enable api
/ip service set [find name="api"] port=8728 address=10.0.0.2/32

# ========================
# 7. CREATE API USER FOR ESP
# ========================
/user add name=$API_USER group=write password=$API_PASS comment="ESP8266 API Access"

# ========================
# 8. CREATE HOTSPOT USER PROFILE (BANDWIDTH LIMITS)
# ========================
/ip hotspot user profile add name=default shared-users=1 rate-limit=2M/2M idle-timeout=5m keepalive-timeout=15m status-autorefresh=1m

# ========================
# 9. MINIMAL LOGIN/LOGOUT SCRIPTS
# ========================
/ip hotspot profile set [find default=yes ] on-login=""
/ip hotspot profile set [find default=yes ] on-logout=""

# ========================
# 10. FIREWALL (OPTIONAL - BLOCK USERS FROM MANAGEMENT)
# ========================
/ip firewall filter add chain=input action=drop src-address=10.0.0.0/24 dst-port=8291,8728,8729 protocol=tcp comment="Block users from WinBox/API"

# ========================
# DONE - VERIFY WITH:
# /ip hotspot print
# /ip hotspot ip-binding print
# /ip hotspot walled-garden print
# /user print
# ========================
