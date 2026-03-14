SUMMARY = "BNO055 IIO Driver"
DESCRIPTION = "Kernel driver for BNO055 IMU sensor"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=6e5a0457b4015d9fb78fd560ed883885"

inherit module

SRC_URI = "file://bno055_iio.c \
           file://bno055_register.h \
           file://Makefile"

S = "${WORKDIR}"
#KERNEL_MODULE_AUTOLOAD += "bno055_iio"