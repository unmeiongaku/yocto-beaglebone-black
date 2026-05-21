#include <lvgl/lvgl.h>
#include <lvgl/src/drivers/display/fb/lv_linux_fbdev.h>

#include <unistd.h>

int main(void)
{
    lv_init();

    /* Create FBDEV display */
    lv_display_t * disp = lv_linux_fbdev_create();

    /* Use framebuffer device */
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    /* Simple label */
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello BBB");
    lv_obj_center(label);

    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}