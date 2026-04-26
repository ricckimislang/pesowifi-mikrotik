# jan/02/1970 01:18:52 by RouterOS 6.49.19
# software id = RSZK-PWHY
#
# model = RB941-2nD
# serial number = HCP08AVW9AD
/interface bridge
add name=bridge-local
/interface wireless
set [ find default-name=wlan1 ] band=2ghz-b/g/n disabled=no frequency=auto \
    mode=ap-bridge ssid=DormitoryWifi
/interface wireless security-profiles
set [ find default=yes ] supplicant-identity=MikroTik
/ip hotspot profile
set [ find default=yes ] hotspot-address=10.0.0.1 login-by=http-chap,http-pap
add hotspot-address=10.0.0.1 login-by=http-chap,http-pap name=hsprof1
/ip hotspot user profile
set [ find default=yes ] idle-timeout=5m keepalive-timeout=15m rate-limit=\
    2M/2M session-timeout=4h
/ip pool
add name=hs-pool ranges=10.0.0.51-10.0.0.200
/ip dhcp-server
add address-pool=hs-pool disabled=no interface=bridge-local lease-time=1h \
    name=hs-dhcp
/ip hotspot
add address-pool=hs-pool addresses-per-mac=1 disabled=no interface=\
    bridge-local name=hotspot1 profile=hsprof1
/interface bridge port
add bridge=bridge-local interface=wlan1
/ip address
add address=10.0.0.1/24 interface=bridge-local network=10.0.0.0
/ip dhcp-client
add disabled=no interface=ether1
/ip dhcp-server network
add address=10.0.0.0/24 dns-server=10.0.0.1 gateway=10.0.0.1
/ip dns
set allow-remote-requests=yes servers=8.8.8.8,8.8.4.4
/ip dns static
add address=10.0.0.1 name=connectivitycheck.gstatic.com
add address=10.0.0.1 name=connectivitycheck.android.com
add address=10.0.0.1 name=captive.apple.com
add address=10.0.0.1 name=www.apple.com
add address=10.0.0.1 name=www.msftconnecttest.com
add address=10.0.0.1 name=msftconnecttest.com
add address=10.0.0.1 name=www.gstatic.com
/ip firewall filter
add action=passthrough chain=unused-hs-chain comment=\
    "place hotspot rules here" disabled=yes
add action=accept chain=input comment="Allow ESP Telnet access" dst-port=23 \
    in-interface=bridge-local protocol=tcp src-address=10.0.0.2
add action=drop chain=input comment=\
    "Block hotspot users from management ports" dst-port=8291,23 \
    in-interface=bridge-local protocol=tcp
/ip firewall nat
add action=passthrough chain=unused-hs-chain comment=\
    "place hotspot rules here" disabled=yes
add action=masquerade chain=srcnat out-interface=ether1
/ip hotspot ip-binding
add address=10.0.0.2 comment="ESP8266 Controller" mac-address=\
    AA:BB:CC:DD:EE:FF type=bypassed
/ip hotspot walled-garden
add comment="place hotspot rules here" disabled=yes
add dst-host=connectivitycheck.gstatic.com
add dst-host=connectivitycheck.android.com
add dst-host=captive.apple.com
add dst-host=www.msftconnecttest.com
add dst-host=msftconnecttest.com
/ip hotspot walled-garden ip
add action=accept comment="Portal IP" disabled=no dst-address=10.0.0.1
add action=accept comment="ESP IP" disabled=no dst-address=10.0.0.2
/ip service
set telnet address=10.0.0.2/32
