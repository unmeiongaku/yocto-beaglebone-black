.PHONY := wifi wificonnect wifi-off clean

SSID = WIFI_NAME
PASS = PASSWORD

CC = gcc
APP = app

SOURCE_NAME = user-wifi-connect
TARGET = $(APP)-$(SOURCE_NAME)
SRC = $(SOURCE_NAME).c

wificonnect:
	$(CC) $(SRC) -o $(TARGET)
 	
connect: 
	dhcpcd wlan0

wifi:
	ip link set wlan0 up
	wpa_passphrase "$(SSID)" "$(PASS)" > /etc/wpa_supplicant.conf
	wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf
	dhcpcd wlan0

wifi-off:
	killall wpa_supplicant
	ip link set wlan0 down

clean: wifi-off
	rm -rf $(TARGET)
#sudo make -f wifi wifi SSID="iPhone" PASS="12345689"