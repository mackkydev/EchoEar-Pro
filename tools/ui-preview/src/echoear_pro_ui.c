#include "echoear_pro_ui.h"

void echoear_pro_ui_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B1020), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "EchoEar Pro");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "AI Companion");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x7E8AA8), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 38);

    lv_obj_t *left_eye = lv_obj_create(scr);
    lv_obj_set_size(left_eye, 74, 46);
    lv_obj_set_style_radius(left_eye, 24, 0);
    lv_obj_set_style_bg_color(left_eye, lv_color_hex(0x32D3FF), 0);
    lv_obj_set_style_border_width(left_eye, 0, 0);
    lv_obj_align(left_eye, LV_ALIGN_CENTER, -52, -26);

    lv_obj_t *right_eye = lv_obj_create(scr);
    lv_obj_set_size(right_eye, 74, 46);
    lv_obj_set_style_radius(right_eye, 24, 0);
    lv_obj_set_style_bg_color(right_eye, lv_color_hex(0x32D3FF), 0);
    lv_obj_set_style_border_width(right_eye, 0, 0);
    lv_obj_align(right_eye, LV_ALIGN_CENTER, 52, -26);

    lv_obj_t *mouth = lv_obj_create(scr);
    lv_obj_set_size(mouth, 92, 10);
    lv_obj_set_style_radius(mouth, 10, 0);
    lv_obj_set_style_bg_color(mouth, lv_color_hex(0x32D3FF), 0);
    lv_obj_set_style_border_width(mouth, 0, 0);
    lv_obj_align(mouth, LV_ALIGN_CENTER, 0, 28);

    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "Listening...");
    lv_obj_set_style_text_color(status, lv_color_hex(0xAAB6D3), 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 58);

    lv_obj_t *bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 180, 10);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 65, LV_ANIM_OFF);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -24);

    lv_obj_t *mode = lv_label_create(scr);
    lv_label_set_text(mode, "MODE: DESKTOP");
    lv_obj_set_style_text_color(mode, lv_color_hex(0x56627F), 0);
    lv_obj_align(mode, LV_ALIGN_BOTTOM_MID, 0, -6);
}