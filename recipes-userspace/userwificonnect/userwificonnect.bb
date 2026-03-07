MPDIR = "${TOPDIR}/tmp"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DESCRIPTION = "Recipes Wifi Connection"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=4a0f8ad6a793571b331b0e19e3dd925c"

SRC_URI += "file://wifi.mk \
           file://user-wifi-connect.c \
           file://COPYING \
          "

S = "${WORKDIR}"

do_install() {
    install -d ${D}/usr/wifi-connect
    install -m 0644 wifi.mk ${D}/usr/wifi-connect/
    install -m 0644 user-wifi-connect.c ${D}/usr/wifi-connect/
}

FILES:${PN} += "/usr/wifi-connect"