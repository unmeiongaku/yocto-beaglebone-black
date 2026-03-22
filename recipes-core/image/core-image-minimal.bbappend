# User "root" has password set to "soundmax" in the image.
# printf "%q" $(mkpasswd -m sha256crypt soundmax)
# $6$c1quXVPsDnvPVqWu$Tk1n/bfmNNO4OT0YiP3HsHU7Y6s2/G9ivdK.jDvgTqqdawpCDY4EnyXZArkbtM5xVz8nNdbpFbFfFdxhYQdOX1
inherit extrausers
TMPDIR = "${TOPDIR}/tmp"

PASSWD = "\$6\$c1quXVPsDnvPVqWu\$Tk1n/bfmNNO4OT0YiP3HsHU7Y6s2/G9ivdK.jDvgTqqdawpCDY4EnyXZArkbtM5xVz8nNdbpFbFfFdxhYQdOX1"
EXTRA_USERS_PARAMS = "\
    usermod -p '${PASSWD}' root; \
    "

IMAGE_INSTALL:append = " banner"

IMAGE_OVERHEAD_FACTOR = "1.2"
IMAGE_ROOTFS_SIZE = "4194304"   
IMAGE_ROOTFS_MAXSIZE = "8388608" 

#USB WiFi package
IMAGE_INSTALL:append = " \
    kernel-module-rtl8188eu \
    linux-firmware \
    dhcpcd \
    iw \
    wpa-supplicant \
    wireless-regdb-static \
    net-tools \
"

#SSH VSCode server package
IMAGE_INSTALL:append = " \
    dhcpcd \
    iproute2 \
    iputils \
    openssh \
    bash \
    tar \
    xz \
    procps \
    coreutils \
    curl \
    libgcc \
    libstdc++ \
    libatomic \
"

IMAGE_INSTALL:append = " \
    packagegroup-core-buildessential \
    nano \
    i2c-tools \
    opkg \
    dtc \
    kernel-modules \
    kernel-dev \
    kernel-devsrc \
    tree \
    fbset \
    con2fbmap \
"
IMAGE_INSTALL:append = " util-linux"

#DEBUG GROUP
IMAGE_INSTALL:append = " \
    strace \
    ltrace \
    gdb \
    gdbserver \
    lsof \
"
#filesystem + disk tool
IMAGE_INSTALL:append = " \
    e2fsprogs \
    dosfstools \
    parted \
"
#log system
IMAGE_INSTALL:append = " \
    htop \
    less \
    file \
"

IMAGE_INSTALL:append = " \
    libgpiod \
    libgpiod-tools \
"

IMAGE_INSTALL:append = " \
    spidev-test \
"

KERNEL_MODULE_AUTOLOAD:append = " \
    rtl8188eu \
"

IMAGE_INSTALL:append = " wpa-supplicant iw dhcpcd"
CORE_IMAGE_EXTRA_INSTALL += " packagegroup-base-wifi kernel-modules"

IMAGE_INSTALL:append = " hello"
IMAGE_INSTALL:append = " ll-character-device"
IMAGE_INSTALL:append = " userspace"
IMAGE_INSTALL:append = " userwificonnect"

#IMAGE_INSTALL:append = " bno055"