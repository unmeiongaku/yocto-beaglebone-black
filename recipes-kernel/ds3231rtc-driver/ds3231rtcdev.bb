SUMMARY = "DS3231 Real Time Clock Driver"
DESCRIPTION = "Linux kernel module for DS3231RTC"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=4a0f8ad6a793571b331b0e19e3dd925c"

inherit module

SRC_URI = "file://ds3231rtc_core.c \
           file://ds3231rtc.h \
           file://COPYING \
           file://Makefile \
           file://Kconfig"

S = "${WORKDIR}"

KERNEL_MODULE_AUTOLOAD +="ds3231rtcdev"