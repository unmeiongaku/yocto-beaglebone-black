SUMMARY = "Simple LVGL UI"
DESCRIPTION = "LVGL FBDEV ThorVG test application"

LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=4a0f8ad6a793571b331b0e19e3dd925c"

SRC_URI = " \
    file://main.c \
    file://test.svg \
    file://CMakeLists.txt \
    file://COPYING \
"

S = "${WORKDIR}"

DEPENDS += " \
    lvgl \
    pkgconfig-native \
"

inherit cmake pkgconfig

do_install() {

    # Install executable
    install -d ${D}${bindir}
    install -m 0755 my-ui ${D}${bindir}

    # Install SVG
    install -d ${D}/usr/share
    install -m 0644 \
        ${WORKDIR}/test.svg \
        ${D}/usr/share/test.svg
}

FILES:${PN} += "/usr/share/test.svg"