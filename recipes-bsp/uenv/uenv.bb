DESCRIPTION = "Custom uEnv"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=4a0f8ad6a793571b331b0e19e3dd925c"


SRC_URI = "file://uEnv.txt \
           file://COPYING \
           "

S = "${WORKDIR}"

do_install() {
    install -d ${D}/boot
    install -m 0644 uEnv.txt ${D}/boot/
}

FILES:${PN} += "/boot/uEnv.txt"