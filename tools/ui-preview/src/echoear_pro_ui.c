#include "echoear_pro_ui.h"
#include <stdbool.h>

#define COLOR_BG        0x000000
#define COLOR_PANEL     0x05070D
#define COLOR_WHITE     0xF4F7FF
#define COLOR_WHITE_DIM 0xAEB6C8
#define COLOR_CYAN      0x43F5E8
#define COLOR_CYAN_DIM  0x155A63
#define COLOR_DARK_LINE 0x10131C

static lv_obj_t *title_label;
static lv_obj_t *subtitle_label;

static lv_obj_t *left_glow;
static lv_obj_t *right_glow;
static lv_obj_t *mouth_glow;

static lv_obj_t *left_eye;
static lv_obj_t *right_eye;
static lv_obj_t *mouth;

static lv_obj_t *status_label;
static lv_obj_t *mode_label;
static lv_obj_t *bar;

static lv_obj_t *car_panel;
static lv_obj_t *car_line_1;
static lv_obj_t *car_line_2;
static lv_obj_t *car_line_3;
static lv_obj_t *car_line_4;

static lv_obj_t *left_scan[5];
static lv_obj_t *right_scan[5];
static lv_obj_t *mouth_scan[4];

static lv_obj_t *make_box(lv_obj_t *parent, int w, int h, uint32_t color, int radius, lv_opa_t opa)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    return obj;
}

static void apply_glow(lv_obj_t *obj, uint32_t color, lv_opa_t opa)
{
    lv_obj_set_style_shadow_width(obj, 24, 0);
    lv_obj_set_style_shadow_spread(obj, 2, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_shadow_opa(obj, opa, 0);
}

static void create_scanlines(lv_obj_t *parent, lv_obj_t **lines, int count)
{
    for(int i = 0; i < count; i++) {
        lines[i] = make_box(parent, 120, 2, COLOR_DARK_LINE, 1, LV_OPA_30);
        lv_obj_align(lines[i], LV_ALIGN_CENTER, 0, -12 + (i * 6));
    }
}

static void set_scan_visible(lv_obj_t **lines, int count, bool visible)
{
    for(int i = 0; i < count; i++) {
        if(visible) lv_obj_remove_flag(lines[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(lines[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void set_led_color(uint32_t color, lv_opa_t opa)
{
    lv_obj_set_style_bg_color(left_eye, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(right_eye, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(mouth, lv_color_hex(color), 0);

    lv_obj_set_style_bg_opa(left_eye, opa, 0);
    lv_obj_set_style_bg_opa(right_eye, opa, 0);
    lv_obj_set_style_bg_opa(mouth, opa, 0);

    lv_obj_set_style_bg_color(left_glow, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(right_glow, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(mouth_glow, lv_color_hex(color), 0);

    apply_glow(left_eye, color, LV_OPA_70);
    apply_glow(right_eye, color, LV_OPA_70);
    apply_glow(mouth, color, LV_OPA_50);
}

static void set_face_shape(
    int eye_w, int eye_h,
    int mouth_w, int mouth_h,
    int eye_radius, int mouth_radius,
    int eye_y, int mouth_y
)
{
    lv_obj_set_size(left_glow, eye_w + 18, eye_h + 14);
    lv_obj_set_size(right_glow, eye_w + 18, eye_h + 14);
    lv_obj_set_size(mouth_glow, mouth_w + 16, mouth_h + 10);

    lv_obj_set_size(left_eye, eye_w, eye_h);
    lv_obj_set_size(right_eye, eye_w, eye_h);
    lv_obj_set_size(mouth, mouth_w, mouth_h);

    lv_obj_set_style_radius(left_glow, eye_radius + 8, 0);
    lv_obj_set_style_radius(right_glow, eye_radius + 8, 0);
    lv_obj_set_style_radius(mouth_glow, mouth_radius + 6, 0);

    lv_obj_set_style_radius(left_eye, eye_radius, 0);
    lv_obj_set_style_radius(right_eye, eye_radius, 0);
    lv_obj_set_style_radius(mouth, mouth_radius, 0);

    lv_obj_align(left_glow, LV_ALIGN_CENTER, -52, eye_y);
    lv_obj_align(right_glow, LV_ALIGN_CENTER, 52, eye_y);
    lv_obj_align(mouth_glow, LV_ALIGN_CENTER, 0, mouth_y);

    lv_obj_align(left_eye, LV_ALIGN_CENTER, -52, eye_y);
    lv_obj_align(right_eye, LV_ALIGN_CENTER, 52, eye_y);
    lv_obj_align(mouth, LV_ALIGN_CENTER, 0, mouth_y);
}

static void set_car_panel_visible(bool visible)
{
    if(visible) {
        lv_obj_remove_flag(car_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(car_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
    }
}

void echoear_pro_ui_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    title_label = lv_label_create(scr);
    lv_label_set_text(title_label, "EchoEar Pro");
    lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 12);

    subtitle_label = lv_label_create(scr);
    lv_label_set_text(subtitle_label, "AI Companion");
    lv_obj_set_style_text_color(subtitle_label, lv_color_hex(0x6E7488), 0);
    lv_obj_align(subtitle_label, LV_ALIGN_TOP_MID, 0, 34);

    left_glow = make_box(scr, 90, 58, COLOR_WHITE, 30, LV_OPA_20);
    right_glow = make_box(scr, 90, 58, COLOR_WHITE, 30, LV_OPA_20);
    mouth_glow = make_box(scr, 96, 18, COLOR_WHITE, 12, LV_OPA_20);

    left_eye = make_box(scr, 64, 38, COLOR_WHITE, 22, LV_OPA_COVER);
    right_eye = make_box(scr, 64, 38, COLOR_WHITE, 22, LV_OPA_COVER);
    mouth = make_box(scr, 76, 10, COLOR_WHITE, 8, LV_OPA_COVER);

    create_scanlines(left_eye, left_scan, 5);
    create_scanlines(right_eye, right_scan, 5);
    create_scanlines(mouth, mouth_scan, 4);

    status_label = lv_label_create(scr);
    lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_WHITE_DIM), 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 58);

    bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 180, 8);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x1C2233), 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(COLOR_WHITE), LV_PART_INDICATOR);

    mode_label = lv_label_create(scr);
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0x626B80), 0);
    lv_obj_align(mode_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    car_panel = lv_obj_create(scr);
    lv_obj_remove_style_all(car_panel);
    lv_obj_set_size(car_panel, 270, 72);
    lv_obj_align(car_panel, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_radius(car_panel, 12, 0);
    lv_obj_set_style_bg_color(car_panel, lv_color_hex(0x020A10), 0);
    lv_obj_set_style_bg_opa(car_panel, LV_OPA_80, 0);
    lv_obj_set_style_border_width(car_panel, 1, 0);
    lv_obj_set_style_border_color(car_panel, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_set_style_border_opa(car_panel, LV_OPA_70, 0);
    apply_glow(car_panel, COLOR_CYAN, LV_OPA_30);

    car_line_1 = lv_label_create(car_panel);
    lv_label_set_text(car_line_1, "OBD READY");
    lv_obj_set_style_text_color(car_line_1, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_align(car_line_1, LV_ALIGN_TOP_LEFT, 12, 8);

    car_line_2 = lv_label_create(car_panel);
    lv_label_set_text(car_line_2, "SOC 82%");
    lv_obj_set_style_text_color(car_line_2, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_align(car_line_2, LV_ALIGN_TOP_LEFT, 12, 30);

    car_line_3 = lv_label_create(car_panel);
    lv_label_set_text(car_line_3, "RANGE 478km");
    lv_obj_set_style_text_color(car_line_3, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_align(car_line_3, LV_ALIGN_TOP_RIGHT, -12, 30);

    car_line_4 = lv_label_create(car_panel);
    lv_label_set_text(car_line_4, "SPEED 0 km/h");
    lv_obj_set_style_text_color(car_line_4, lv_color_hex(0x9CFDF5), 0);
    lv_obj_align(car_line_4, LV_ALIGN_BOTTOM_MID, 0, -8);

    echoear_pro_ui_set_state(ECHOEAR_STATE_IDLE);
}

void echoear_pro_ui_set_state(echoear_state_t state)
{
    set_car_panel_visible(false);
    set_scan_visible(left_scan, 5, true);
    set_scan_visible(right_scan, 5, true);
    set_scan_visible(mouth_scan, 4, true);

    switch(state) {
        case ECHOEAR_STATE_IDLE:
            lv_label_set_text(status_label, "Idle");
            lv_label_set_text(mode_label, "MODE: DESKTOP");
            set_led_color(COLOR_WHITE, LV_OPA_COVER);
            set_face_shape(58, 38, 70, 8, 20, 8, -26, 28);
            lv_bar_set_value(bar, 25, LV_ANIM_ON);
            break;

        case ECHOEAR_STATE_LISTENING:
            lv_label_set_text(status_label, "Listening...");
            lv_label_set_text(mode_label, "MODE: VOICE");
            set_led_color(COLOR_WHITE, LV_OPA_COVER);
            set_face_shape(72, 34, 92, 8, 18, 8, -28, 30);
            lv_bar_set_value(bar, 70, LV_ANIM_ON);
            break;

        case ECHOEAR_STATE_THINKING:
            lv_label_set_text(status_label, "Thinking...");
            lv_label_set_text(mode_label, "MODE: AI");
            set_led_color(COLOR_WHITE, LV_OPA_COVER);
            lv_obj_set_size(left_eye, 48, 42);
            lv_obj_set_size(right_eye, 58, 12);
            lv_obj_set_size(left_glow, 66, 56);
            lv_obj_set_size(right_glow, 76, 24);
            lv_obj_set_size(mouth, 22, 10);
            lv_obj_set_size(mouth_glow, 36, 22);
            lv_obj_align(left_eye, LV_ALIGN_CENTER, -52, -28);
            lv_obj_align(right_eye, LV_ALIGN_CENTER, 52, -28);
            lv_obj_align(left_glow, LV_ALIGN_CENTER, -52, -28);
            lv_obj_align(right_glow, LV_ALIGN_CENTER, 52, -28);
            lv_obj_align(mouth, LV_ALIGN_CENTER, 0, 28);
            lv_obj_align(mouth_glow, LV_ALIGN_CENTER, 0, 28);
            lv_bar_set_value(bar, 45, LV_ANIM_ON);
            break;

        case ECHOEAR_STATE_SPEAKING:
            lv_label_set_text(status_label, "Speaking...");
            lv_label_set_text(mode_label, "MODE: TALK");
            set_led_color(COLOR_WHITE, LV_OPA_COVER);
            set_face_shape(62, 32, 86, 24, 16, 14, -30, 30);
            lv_bar_set_value(bar, 88, LV_ANIM_ON);
            break;

        case ECHOEAR_STATE_SLEEPING:
            lv_label_set_text(status_label, "Sleeping...");
            lv_label_set_text(mode_label, "MODE: SLEEP");
            set_led_color(COLOR_WHITE, LV_OPA_70);
            set_face_shape(62, 8, 54, 5, 4, 4, -25, 30);
            set_scan_visible(left_scan, 5, false);
            set_scan_visible(right_scan, 5, false);
            set_scan_visible(mouth_scan, 4, false);
            lv_bar_set_value(bar, 8, LV_ANIM_ON);
            break;

        case ECHOEAR_STATE_CAR_MODE:
            lv_label_set_text(status_label, "Car Mode");
            lv_label_set_text(mode_label, "OBD: READY");
            set_led_color(COLOR_CYAN, LV_OPA_COVER);
            set_face_shape(58, 24, 38, 8, 14, 6, -48, -18);
            set_car_panel_visible(true);
            lv_bar_set_value(bar, 100, LV_ANIM_ON);
            break;
    }
}
