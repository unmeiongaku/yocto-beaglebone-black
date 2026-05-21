FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Disable DRM backend
PACKAGECONFIG:remove = "drm"

# Enable framebuffer backend
PACKAGECONFIG:append = " fbdev"

# Explicit values (optional)
LVGL_CONFIG_USE_DRM = "0"
LVGL_CONFIG_USE_FBDEV = "1"
LVGL_CONFIG_USE_EVDEV = "1"


do_configure:append() {
    sed -r \
        -e "s|^([[:space:]]*#define LV_USE_VECTOR_GRAPHIC[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_THORVG_INTERNAL[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_FS_STDIO[[:space:]]).*|\1 1|" \
        -i "${S}/lv_conf.h"
}