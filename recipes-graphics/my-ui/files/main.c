#include <lvgl/lvgl.h>
#include <lvgl/src/drivers/display/fb/lv_linux_fbdev.h>

#include <unistd.h>

int main(void)
{
    lv_init();

    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    lv_obj_set_style_bg_color(
        lv_screen_active(),
        lv_color_hex(0x202020),
        0
    );

    lv_obj_t * obj = lv_obj_create(lv_screen_active());

    lv_obj_set_size(obj, 200, 200);

    lv_obj_set_style_radius(obj, 40, 0);

    lv_obj_set_style_bg_color(
        obj,
        lv_color_hex(0x0000ff),
        0
    );

    lv_obj_set_style_shadow_width(obj, 40, 0);

    lv_obj_set_style_shadow_color(
        obj,
        lv_color_hex(0xff0000),
        0
    );

    lv_obj_center(obj);

    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}