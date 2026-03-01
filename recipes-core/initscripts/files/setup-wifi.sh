ip link set wlan0 up
wpa_passphrase "Hoang Minh" "hoangminh2000" > /etc/wpa_supplicant.conf
wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf
dhcpcd wlan0
