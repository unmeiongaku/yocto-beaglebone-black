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
    sed -r -i \
        -e "s|^([[:space:]]*#define LV_COLOR_DEPTH[[:space:]]).*|\1 32|" \
        -e "s|^([[:space:]]*#define LV_MEM_SIZE[[:space:]]*).*$|\1(256 * 1024U)|" \
        -e "s|^([[:space:]]*#define LV_USE_LOG[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_LOG_PRINTF[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_ASSERT_NULL[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_ASSERT_MALLOC[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_8[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_10[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_12[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_14[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_16[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_18[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_20[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_22[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_24[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_26[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_28[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_30[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_32[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_34[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_36[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_38[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_40[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_42[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_42[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_44[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_46[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_48[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_UNSCII_8[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_UNSCII_16[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_FONT_FMT_TXT_LARGE[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_FONT_COMPRESSED[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_FS_STDIO[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_FS_MEMFS[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_LODEPNG[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_BMP[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_TJPGD[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_GIF[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_RLE[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_QRCODE[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_BARCODE[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_TINY_TTF[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_VECTOR_GRAPHIC[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_THORVG_INTERNAL[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_FS_STDIO[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_LZ4_INTERNAL[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_SNAPSHOT[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_SYSMON[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_MONKEY[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_GRIDNAV[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_FRAGMENT[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_IMGFONT[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_IME_PINYIN[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_USE_FILE_EXPLORER[[:space:]]).*|\1 1|" \
        -e "s|^([[:space:]]*#define LV_LINUX_FBDEV_BUFFER_COUNT[[:space:]]+).*|\1 2|" \
        -e "s|^([[:space:]]*#define LV_USE_EVDEV[[:space:]]).*|\1 1|" \
        "${S}/lv_conf.h"

    sed -i \
        '/#define LV_USE_IMGFONT 1/c\
        /*1: Support using images as font in label or span widgets */\
        #if LV_USE_IMGFONT\
            /*Imgfont image file path maximum length*/\
            #define LV_IMGFONT_PATH_MAX_LEN 64\
        \
            /*1: Use img cache to buffer header information*/\
            #define LV_IMGFONT_USE_IMAGE_CACHE_HEADER 0\
        #endif' \
                "${S}/lv_conf.h"
}