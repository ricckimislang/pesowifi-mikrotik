# Captive Portal Test Script
# Run this to verify your hotspot setup is working correctly

# ========================
# CHECK HOTSPOT STATUS
# ========================
:log info "=== Checking Hotspot Status ==="
/ip hotspot print
:delay 2s

# ========================
# CHECK DHCP LEASES
# ========================
:log info "=== Checking DHCP Leases ==="
/ip dhcp-server lease print
:delay 2s

# ========================
# CHECK WALLED GARDEN
# ========================
:log info "=== Checking Walled Garden Rules ==="
/ip hotspot walled-garden print
:delay 2s

# ========================
# CHECK IP BINDINGS
# ========================
:log info "=== Checking IP Bindings ==="
/ip hotspot ip-binding print
:delay 2s

# ========================
# CHECK SERVICES
# ========================
:log info "=== Checking Services ==="
/ip service print where name~"telnet|www|api"
:delay 2s

# ========================
# TEST PORTAL ACCESS
# ========================
:log info "=== Testing Portal Access ==="
/tool ping address=10.0.0.1 count=3
:delay 2s

# ========================
# CHECK ACTIVE USERS
# ========================
:log info "=== Current Active Users ==="
/ip hotspot active print
:delay 2s

# ========================
# MONITOR NEW CONNECTIONS (10 seconds)
# ========================
:log info "=== Monitoring Connections (10 seconds) ==="
:for i from=1 to=10 do={
    /ip hotspot active print brief
    :delay 1s
}

:log info "=== Captive Portal Test Complete ==="
:log info "Connect your mobile device and try to browse the internet"
:log info "You should be redirected to the login page"
