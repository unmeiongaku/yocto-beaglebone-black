#!/bin/sh

# ===== COLOR =====
RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BLUE="\033[1;34m"
CYAN="\033[1;36m"
WHITE="\033[1;37m"
NC="\033[0m"

BANNERCOLOUR=$BLUE

# ===== SENSOR =====
TEMP_RAW=$(cat /sys/bus/iio/devices/iio:device1/in_temp_raw)
TEMP_SCALE=$(cat /sys/bus/iio/devices/iio:device1/in_temp_scale)

PRESS_RAW=$(cat /sys/bus/iio/devices/iio:device1/in_pressure_raw)
PRESS_SCALE=$(cat /sys/bus/iio/devices/iio:device1/in_pressure_scale)

TEMP=$(awk "BEGIN {printf \"%.2f\", $TEMP_RAW * $TEMP_SCALE}")
PRESS=$(awk "BEGIN {printf \"%.2f\", $PRESS_RAW * $PRESS_SCALE}")

PRESS_HPA=$(awk "BEGIN {printf \"%.2f\", $PRESS / 1}")
SEA=1013.25

ALT=$(awk "BEGIN {printf \"%.2f\", 44330 * (1.0 - ($PRESS_HPA / $SEA)^0.1903)}")

# ===== TEMP COLOR =====
if awk "BEGIN {exit !($TEMP > 60)}"; then
    TEMP_COLOR=$RED
elif awk "BEGIN {exit !($TEMP > 40)}"; then
    TEMP_COLOR=$YELLOW
else
    TEMP_COLOR=$GREEN
fi

# ===== OUTPUT =====
echo -e "
${CYAN}╔══════════════════════════════════════════════════╗
║${NC} ${BANNERCOLOUR} ____  _____ ____  __  __ _____ ___ _   ___   __${NC} ${CYAN}║
║${NC} ${BANNERCOLOUR}|  _ \\| ____/ ___||  \\/  |_   _|_ _| \\ | \\ \\ / /${NC} ${CYAN}║
║${NC} ${BANNERCOLOUR}| | | |  _| \\___ \\| |\\/| | | |  | ||  \\| |\\ V / ${NC} ${CYAN}║
║${NC} ${BANNERCOLOUR}| |_| | |___ ___) | |  | | | |  | || |\\  | | | ${NC} ${CYAN} ║
║${NC} ${BANNERCOLOUR}|____/|_____|____/|_|  |_| |_| |___|_| \\_| |_|${NC} ${CYAN}  ║
║                                              	   ║
║ ${WHITE}Hostname  ${NC}: $(hostname)                     	   ║
║ ${WHITE}Uptime    ${NC}: $(uptime -p)                  	   ║
║ ${WHITE}Time      ${NC}: $(cat /sys/bus/i2c/devices/2-0068/time) 		   ║
║                                              	   ║
║ ${WHITE}Disk      ${NC}: $(df -h | awk '/\/$/ {print $3 " / " $2}')               	   ║
║ ${WHITE}Memory    ${NC}: $(free | awk '/Mem:/ {printf "%.1fM / %.1fM", $3/1024, $2/1024}') 			   ║
║                                                  ║
║ ${WHITE}Sensors   ${NC}: ${TEMP_COLOR}${TEMP}°C${NC} | ${BLUE}${PRESS} Pa${NC} | ${GREEN}${ALT} m${NC}      ║
${CYAN}╚══════════════════════════════════════════════════╝${NC}
"
