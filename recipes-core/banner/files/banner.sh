#!/bin/sh
TEMP_RAW=$(cat /sys/bus/iio/devices/iio:device1/in_temp_raw)
TEMP_SCALE=$(cat /sys/bus/iio/devices/iio:device1/in_temp_scale)

PRESS_RAW=$(cat /sys/bus/iio/devices/iio:device1/in_pressure_raw)
PRESS_SCALE=$(cat /sys/bus/iio/devices/iio:device1/in_pressure_scale)

TEMP=$(awk "BEGIN {printf \"%.2f\", $TEMP_RAW * $TEMP_SCALE}")
PRESS=$(awk "BEGIN {printf \"%.2f\", $PRESS_RAW * $PRESS_SCALE}")

PRESS_HPA=$(awk "BEGIN {printf \"%.2f\", $PRESS / 1}")
SEA=1013.25

ALT=$(awk "BEGIN {printf \"%.2f\", 44330 * (1.0 - ($PRESS_HPA / $SEA)^0.1903)}")


echo "
  ____  _____ ____  __  __ _____ ___ _   ___   __
 |  _ \| ____/ ___||  \/  |_   _|_ _| \ | \ \ / /
 | | | |  _| \___ \| |\/| | | |  | ||  \| |\ V / 
 | |_| | |___ ___) | |  | | | |  | || |\  | | |  
 |____/|_____|____/|_|  |_| |_| |___|_| \_| |_|  
                                                  
Uptime       : $(uptime)
Hostname     : $(hostname)
Disk Usage   : $(df -h | awk '/\/$/ {print $3 " used of " $2}')
Memory       : $(free | awk '/Mem:/ {printf "%.1fM used of %.1fM", $3/1024, $2/1024}')
Time         : $(cat /sys/bus/i2c/devices/2-0068/time)

Temperature  : ${TEMP} °C
Pressure     : ${PRESS} Pa
Altitute     : ${ALT} m
"