TMPDIR = "${TOPDIR}/tmp"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DESCRIPTION = "Recipes Userspace"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=4a0f8ad6a793571b331b0e19e3dd925c"

SRC_URI += "file://userdevices.mk \
           file://user-bno055-device.c \
           file://COPYING \
          "

S = "${WORKDIR}"

do_install() {
    install -d ${D}/usr/userspace/devices/bno055/
    install -m 0644 userdevices.mk ${D}/usr/userspace/devices/bno055/
    install -m 0644 user-bno055-device.c ${D}/usr/userspace/devices/bno055/
}

FILES:${PN} += "/usr/userspace"