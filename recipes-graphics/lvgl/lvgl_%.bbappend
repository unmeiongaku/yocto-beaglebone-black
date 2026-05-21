FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Disable DRM backend
PACKAGECONFIG:remove = "drm"

# Enable framebuffer backend
PACKAGECONFIG:append = " fbdev"

# Explicit values (optional)
LVGL_CONFIG_USE_DRM = "0"
LVGL_CONFIG_USE_FBDEV = "1"
LVGL_CONFIG_USE_EVDEV = "1"