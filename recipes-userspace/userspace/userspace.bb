TMPDIR = "${TOPDIR}/tmp"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DESCRIPTION = "Recipes Userspace"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=4a0f8ad6a793571b331b0e19e3dd925c"

SRC_URI += "file://ll-custom.mk \
            file://wifi.mk \
           file://user-major-minor.c \
           file://COPYING \
          "

S = "${WORKDIR}"

do_install() {
    install -d ${D}/usr/wifi
    install -m 0644 wifi.mk ${D}/usr/wifi/

    install -d ${D}/usr/userspace/device-character/major-minor

    install -m 0644 ll-custom.mk ${D}/usr/userspace/device-character/

    install -m 0644 user-major-minor.c ${D}/usr/userspace/device-character/major-minor/
}

FILES:${PN} += "/usr/wifi"
FILES:${PN} += "/usr/userspace"