SUMMARY = "ST7735 TFT LCD"
DESCRIPTION = "Linux kernel module for ST7735 TFT LCD"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=d41d8cd98f00b204e9800998ecf8427e"

inherit module

SRC_URI += "file://st7735r_core.c \
            file://COPYING \
            file://Makefile"

S = "${WORKDIR}"

KERNEL_MODULE_AUTOLOAD += "st7735rdev"