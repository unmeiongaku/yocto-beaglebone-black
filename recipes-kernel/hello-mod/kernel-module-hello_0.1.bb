SUMMARY = "Example of how to build an external Linux kernel module"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6e5a0457b4015d9fb78fd560ed883885"

inherit module

SRC_URI = "file://Makefile \
           file://hello.c \
           file://COPYING \
          "

S = "${WORKDIR}"

KERNEL_MODULE_AUTOLOAD += "hello"

do_install() {
    #install -d ${D}/root/workspace/hello
    #install -m 0644 ${B}/hello.ko ${D}/root/workspace/hello/
    install -d ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/workspace/hello
    install -m 0644 hello.ko ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/workspace/hello/
}

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.

FILES:${PN} += "${nonarch_base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/workspace/hello/hello.ko"
RPROVIDES_${PN} += "kernel-module-hello"
