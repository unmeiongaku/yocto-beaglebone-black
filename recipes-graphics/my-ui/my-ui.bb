SUMMARY = "Simple LVGL UI"
LICENSE = "CLOSED"

SRC_URI = " \
    file://main.c \
    file://CMakeLists.txt \
    file://COPYING \
"

S = "${WORKDIR}"

DEPENDS += " \
    lvgl \
    libdrm \
    pkgconfig-native \
"

inherit cmake pkgconfig

do_install() {
    install -d ${D}${bindir}
    install -m 0755 my-ui ${D}${bindir}
}