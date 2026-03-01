TMPDIR = "${TOPDIR}/tmp"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DESCRIPTION = "desmtiny wifi auto connect"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://setup-wifi.sh"

S = "${WORKDIR}"

do_install () {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 setup-wifi.sh ${D}${sysconfdir}/init.d/
}