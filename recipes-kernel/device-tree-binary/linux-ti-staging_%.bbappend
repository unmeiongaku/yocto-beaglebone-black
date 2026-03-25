FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://devdtb.dts \
    file://i2c-devices.dtsi \
"

KERNEL_DEVICETREE += "devdtb.dtb"