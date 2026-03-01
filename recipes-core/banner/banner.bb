TMPDIR = "${TOPDIR}/tmp"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DESCRIPTION = "desmtiny login banner with system info"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://banner.sh"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/profile.d
    install -m 0755 banner.sh ${D}${sysconfdir}/profile.d/
}

