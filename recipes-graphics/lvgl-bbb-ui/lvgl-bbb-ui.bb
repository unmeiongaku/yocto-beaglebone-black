SUMMARY = "LVGL BeagleBone UI Application"
HOMEPAGE = "https://github.com/unmeiongaku/lvgl-yocto-ui"
DESCRIPTION = "LVGL framebuffer application for BeagleBone Black"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=9eb0fc812c1090abf8c3b184741325f5"

SRC_URI = "\
    git://github.com/unmeiongaku/lvgl-yocto-ui.git;protocol=https;branch=master;name=ui \
    git://github.com/lvgl/lvgl.git;protocol=https;branch=master;name=lvgl;destsuffix=git/lvgl \
    file://LICENSE \
"
SRCREV_ui = "10414905d8fc16c96352cff059923ad381c31123"
SRCREV_lvgl = "90f374e8c264b444af7ccf0a236b43911ca69c12"
SRCREV_FORMAT = "ui_lvgl"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

require lv-conf.inc

EXTRA_OECMAKE += "-DCMAKE_BUILD_TYPE=Release"

LVGL_CONFIG_DRM_CARD ?= "/dev/dri/card0"

LVGL_CONFIG_LV_USE_LOG = "1"
LVGL_CONFIG_LV_LOG_PRINTF = "1"

LVGL_CONFIG_LV_MEM_SIZE = "(256 * 1024U)"

LVGL_CONFIG_LV_USE_FONT_COMPRESSED = "1"

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/bin/main ${D}${bindir}/lvgl
}