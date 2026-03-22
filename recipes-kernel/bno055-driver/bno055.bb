SUMMARY = "Bosch BNO055 IIO driver"
DESCRIPTION = "Linux kernel module for BNO055 IMU sensor"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=4a0f8ad6a793571b331b0e19e3dd925c"

inherit module

SRC_URI = "file://bno055_core.c \
           file://bno055_i2c.c \
           file://bno055.h \
           file://COPYING \
           file://Makefile \
           file://Kconfig"

S = "${WORKDIR}"

KERNEL_MODULE_AUTOLOAD +="bno055"