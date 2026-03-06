.PHONY := wifi wifi-off

SSID = WIFI_NAME
PASS = PASSWORD

wifi:
	ip link set wlan0 up
	wpa_passphrase "$(SSID)" "$(PASS)" > /etc/wpa_supplicant.conf
	wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf
	dhcpcd wlan0

wifi-off:
	killall wpa_supplicant
	ip link set wlan0 down

#sudo make -f wifi wifi SSID="iPhone" PASS="12345689"