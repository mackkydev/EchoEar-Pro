#include "echoear_settings_ui.h"

#include "echoear_settings.h"
#include "echoear_pro_ui.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define COLOR_BLACK 0x000000
#define COLOR_PANEL 0xFFFFFF
#define COLOR_ROW 0xEEF2F3
#define COLOR_ROW_PRESSED 0xCFF4F0
#define COLOR_CYAN 0x007D78
#define COLOR_TEXT 0x101517
#define COLOR_MUTED 0x56666B
#define SETTINGS_HOLD_MS 5000U
#define SETTINGS_HOLD_HINT_MS 2500U
#define SETTINGS_HOLD_MOVE_LIMIT 24
#define SETTINGS_SHORT_TAP_MAX_MS 700U
#define SETTINGS_SAVE_FEEDBACK_MS 800U

static lv_obj_t *s_panel;
static lv_obj_t *s_root_page;
static lv_obj_t *s_display_page;
static lv_obj_t *s_audio_page;
static lv_obj_t *s_car_page;
static lv_obj_t *s_hands_free_page;
static lv_obj_t *s_battery_page;
static lv_obj_t *s_subtitles_page;
static lv_obj_t *s_voice_page;
static lv_obj_t *s_system_page;
static lv_obj_t *s_brightness;
static lv_obj_t *s_volume;
static lv_obj_t *s_car_mode;
static lv_obj_t *s_hands_free;
static lv_obj_t *s_show_battery;
static lv_obj_t *s_show_subtitles;
static lv_obj_t *s_brightness_value;
static lv_obj_t *s_volume_value;
static lv_obj_t *s_root_brightness_value;
static lv_obj_t *s_root_volume_value;
static lv_obj_t *s_root_car_value;
static lv_obj_t *s_root_hands_free_value;
static lv_obj_t *s_root_battery_value;
static lv_obj_t *s_root_subtitles_value;
static lv_obj_t *s_root_voice_value;
static lv_obj_t *s_voice_option_values[ECHOEAR_ASSISTANT_VOICE_COUNT];
static lv_obj_t *s_battery_level_value;
static int s_battery_level;
static bool s_battery_charging;
static bool s_battery_valid;
static lv_obj_t *s_reset_label;
static lv_obj_t *s_hold_overlay;
static lv_obj_t *s_hold_bar;
static lv_timer_t *s_hold_timer;
static lv_obj_t *s_save_feedback;
static lv_timer_t *s_save_timer;
static lv_indev_t *s_hold_indev;
static lv_point_t s_hold_start_point;
static uint32_t s_hold_started_ms;
static bool s_hold_active;
static bool s_panel_visible;
static echoear_settings_ui_short_tap_handler_t s_short_tap_handler;
static echoear_settings_ui_visibility_handler_t s_visibility_handler;

static void show_page(echoear_settings_page_t page);

static void save_feedback_timer(lv_timer_t *timer) {
  lv_timer_pause(timer);
  lv_obj_add_flag(s_save_feedback, LV_OBJ_FLAG_HIDDEN);
  echoear_settings_navigate(ECHOEAR_SETTINGS_PAGE_ROOT);
  echoear_settings_ui_refresh();
  show_page(ECHOEAR_SETTINGS_PAGE_ROOT);
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  return label;
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text, int x, int y,
                             int width, int height, lv_event_cb_t callback,
                             void *user_data, lv_obj_t **label_out) {
  lv_obj_t *button = lv_button_create(parent);
  lv_obj_set_size(button, width, height);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_style_radius(button, 18, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_ROW), 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_ROW_PRESSED),
                            LV_STATE_PRESSED);
  lv_obj_set_style_border_width(button, 0, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);

  lv_obj_t *label = make_label(button, text);
  lv_obj_center(label);
  if (label_out != NULL) {
    *label_out = label;
  }
  return button;
}

static lv_obj_t *make_page(lv_obj_t *parent, const char *title) {
  lv_obj_t *page = lv_obj_create(parent);
  lv_obj_remove_style_all(page);
  lv_obj_set_size(page, 360, 360);
  lv_obj_center(page);
  lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *heading = make_label(page, title);
  lv_obj_set_width(heading, 250);
  lv_obj_set_style_text_align(heading, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 26);
  lv_obj_set_style_text_color(heading, lv_color_hex(COLOR_CYAN), 0);
  return page;
}

static void refresh_value_labels(void) {
  char value[12];
  const echoear_settings_t *settings = echoear_settings_get();

  snprintf(value, sizeof(value), "%u%%", settings->brightness);
  lv_label_set_text(s_brightness_value, value);
  lv_label_set_text(s_root_brightness_value, value);

  snprintf(value, sizeof(value), "%u%%", settings->volume);
  lv_label_set_text(s_volume_value, value);
  lv_label_set_text(s_root_volume_value, value);

  lv_label_set_text(s_root_car_value, settings->car_mode ? "ON" : "OFF");
  lv_label_set_text(s_root_hands_free_value,
                    settings->hands_free ? "ON" : "OFF");
  lv_label_set_text(s_root_battery_value,
                    settings->show_battery ? "ON" : "OFF");
  lv_label_set_text(s_root_subtitles_value,
                    settings->show_subtitles ? "ON" : "OFF");
  lv_label_set_text(s_root_voice_value,
                    echoear_assistant_voice_label(settings->assistant_voice));
  for (int i = 0; i < ECHOEAR_ASSISTANT_VOICE_COUNT; ++i) {
    if (s_voice_option_values[i] != NULL) {
      lv_label_set_text(s_voice_option_values[i],
                        settings->assistant_voice == i ? "SELECTED" : "");
    }
  }

  if (s_battery_level_value != NULL) {
    if (!s_battery_valid) {
      lv_label_set_text(s_battery_level_value, "Battery --");
    } else {
      snprintf(value, sizeof(value), "%d%%", s_battery_level);
      lv_label_set_text(s_battery_level_value, value);
    }
  }
}

static void page_clicked(lv_event_t *event) {
  echoear_settings_page_t page =
      (echoear_settings_page_t)(uintptr_t)lv_event_get_user_data(event);
  if (echoear_settings_navigate(page)) {
    show_page(page);
  }
}

static void back_clicked(lv_event_t *event) {
  LV_UNUSED(event);
  if (echoear_settings_navigate(ECHOEAR_SETTINGS_PAGE_ROOT)) {
    show_page(ECHOEAR_SETTINGS_PAGE_ROOT);
  }
}

static void brightness_changed(lv_event_t *event) {
  echoear_settings_set_brightness(
      (uint8_t)lv_slider_get_value(lv_event_get_target(event)));
  refresh_value_labels();
}

static void volume_changed(lv_event_t *event) {
  echoear_settings_set_volume(
      (uint8_t)lv_slider_get_value(lv_event_get_target(event)));
  refresh_value_labels();
}

static void car_mode_changed(lv_event_t *event) {
  echoear_settings_set_car_mode(
      lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED));
  refresh_value_labels();
}

static void hands_free_changed(lv_event_t *event) {
  echoear_settings_set_hands_free(
      lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED));
  refresh_value_labels();
}

static void show_battery_changed(lv_event_t *event) {
  echoear_settings_set_show_battery(
      lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED));
  refresh_value_labels();
}

static void show_subtitles_changed(lv_event_t *event) {
  echoear_settings_set_show_subtitles(
      lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED));
  refresh_value_labels();
}

static void voice_changed(lv_event_t *event) {
  echoear_assistant_voice_t voice =
      (echoear_assistant_voice_t)(uintptr_t)lv_event_get_user_data(event);
  echoear_settings_set_assistant_voice(voice);
  refresh_value_labels();
}

static void save_clicked(lv_event_t *event) {
  LV_UNUSED(event);
  if (echoear_settings_save()) {
    echoear_pro_ui_set_battery_visible(
        echoear_settings_get()->show_battery);
    if (!echoear_settings_get()->show_subtitles) {
      echoear_pro_ui_set_subtitle("");
    }
    lv_obj_remove_flag(s_save_feedback, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_save_feedback);
    lv_timer_resume(s_save_timer);
    lv_timer_reset(s_save_timer);
  }
}

static void cancel_clicked(lv_event_t *event) {
  LV_UNUSED(event);
  echoear_settings_cancel();
  echoear_settings_ui_hide();
}

static void wifi_clicked(lv_event_t *event) {
  LV_UNUSED(event);
  if (echoear_settings_request_wifi_setup()) {
    echoear_settings_ui_hide();
  }
}

static void reset_clicked(lv_event_t *event) {
  LV_UNUSED(event);
  if (echoear_settings_get()->page != ECHOEAR_SETTINGS_PAGE_RESET_CONFIRM) {
    if (echoear_settings_navigate(ECHOEAR_SETTINGS_PAGE_RESET_CONFIRM)) {
      lv_label_set_text(s_reset_label, "Tap again to reset");
      lv_obj_set_style_text_color(s_reset_label, lv_color_hex(0xFFB4A8), 0);
    }
    return;
  }
  if (echoear_settings_request_factory_reset()) {
    echoear_settings_ui_hide();
  }
}

static void cancel_settings_hold(void) {
  s_hold_active = false;
  s_hold_indev = NULL;
  if (s_hold_timer != NULL) {
    lv_timer_pause(s_hold_timer);
  }
  if (s_hold_overlay != NULL) {
    lv_obj_add_flag(s_hold_overlay, LV_OBJ_FLAG_HIDDEN);
  }
}

static void settings_hold_timer(lv_timer_t *timer) {
  LV_UNUSED(timer);
  if (!s_hold_active) {
    return;
  }

  uint32_t elapsed = lv_tick_get() - s_hold_started_ms;
  if (elapsed >= SETTINGS_HOLD_HINT_MS &&
      lv_obj_has_flag(s_hold_overlay, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_remove_flag(s_hold_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_hold_overlay);
  }

  uint32_t visible_elapsed =
      elapsed > SETTINGS_HOLD_HINT_MS ? elapsed - SETTINGS_HOLD_HINT_MS : 0U;
  int32_t progress = (int32_t)((visible_elapsed * 100U) /
                               (SETTINGS_HOLD_MS - SETTINGS_HOLD_HINT_MS));
  if (progress > 100) {
    progress = 100;
  }
  lv_bar_set_value(s_hold_bar, progress, LV_ANIM_OFF);

  if (elapsed >= SETTINGS_HOLD_MS) {
    lv_indev_t *indev = s_hold_indev;
    cancel_settings_hold();
    if (indev != NULL) {
      lv_indev_wait_release(indev);
    }
    echoear_settings_ui_show();
  }
}

static void settings_hold_event(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_PRESSED) {
    if (!lv_obj_has_flag(s_panel, LV_OBJ_FLAG_HIDDEN)) {
      return;
    }
    s_hold_indev = lv_indev_active();
    if (s_hold_indev == NULL) {
      return;
    }
    lv_indev_get_point(s_hold_indev, &s_hold_start_point);
    s_hold_started_ms = lv_tick_get();
    s_hold_active = true;
    lv_bar_set_value(s_hold_bar, 0, LV_ANIM_OFF);
    lv_obj_add_flag(s_hold_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_timer_resume(s_hold_timer);
    lv_timer_reset(s_hold_timer);
    return;
  }

  if (code == LV_EVENT_PRESSING && s_hold_active && s_hold_indev != NULL) {
    lv_point_t point;
    lv_indev_get_point(s_hold_indev, &point);
    int32_t dx = point.x - s_hold_start_point.x;
    int32_t dy = point.y - s_hold_start_point.y;
    if (dx > SETTINGS_HOLD_MOVE_LIMIT || dx < -SETTINGS_HOLD_MOVE_LIMIT ||
        dy > SETTINGS_HOLD_MOVE_LIMIT || dy < -SETTINGS_HOLD_MOVE_LIMIT) {
      cancel_settings_hold();
    }
    return;
  }

  if (code == LV_EVENT_RELEASED) {
    bool short_tap =
        s_hold_active &&
        (lv_tick_get() - s_hold_started_ms <= SETTINGS_SHORT_TAP_MAX_MS) &&
        lv_obj_has_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    cancel_settings_hold();
    if (short_tap && s_short_tap_handler != NULL) {
      s_short_tap_handler();
    }
    return;
  }

  if (code == LV_EVENT_PRESS_LOST) {
    cancel_settings_hold();
  }
}

static lv_obj_t *make_root_row(lv_obj_t *parent, const char *title,
                               const char *initial_value,
                               lv_event_cb_t callback, void *user_data,
                               lv_obj_t **value_out) {
  lv_obj_t *row = lv_button_create(parent);
  lv_obj_set_width(row, 252);
  lv_obj_set_height(row, 48);
  lv_obj_set_style_radius(row, 18, 0);
  lv_obj_set_style_bg_color(row, lv_color_hex(COLOR_ROW), 0);
  lv_obj_set_style_bg_color(row, lv_color_hex(COLOR_ROW_PRESSED),
                            LV_STATE_PRESSED);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_shadow_width(row, 0, 0);
  lv_obj_add_event_cb(row, callback, LV_EVENT_CLICKED, user_data);

  lv_obj_t *title_label = make_label(row, title);
  lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 2, 0);

  lv_obj_t *value_label = make_label(row, initial_value);
  lv_obj_set_style_text_color(value_label, lv_color_hex(COLOR_CYAN), 0);
  lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -2, 0);
  if (value_out != NULL) {
    *value_out = value_label;
  }
  return row;
}

static void style_slider(lv_obj_t *slider) {
  lv_obj_set_style_bg_color(slider, lv_color_hex(0x24414A), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(COLOR_CYAN),
                            LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_hex(COLOR_TEXT), LV_PART_KNOB);
}

static void make_detail_footer(lv_obj_t *page) {
  make_button(page, "BACK", 62, 250, 104, 42, back_clicked, NULL, NULL);
  make_button(page, "SAVE", 194, 250, 104, 42, save_clicked, NULL, NULL);
}

static void show_page(echoear_settings_page_t page) {
  lv_obj_add_flag(s_root_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_display_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_audio_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_car_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_hands_free_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_battery_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_subtitles_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_voice_page, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_system_page, LV_OBJ_FLAG_HIDDEN);

  switch (page) {
  case ECHOEAR_SETTINGS_PAGE_DISPLAY:
    lv_obj_remove_flag(s_display_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_AUDIO:
    lv_obj_remove_flag(s_audio_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_CAR_MODE:
    lv_obj_remove_flag(s_car_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_HANDS_FREE:
    lv_obj_remove_flag(s_hands_free_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_BATTERY:
    lv_obj_remove_flag(s_battery_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_SUBTITLES:
    lv_obj_remove_flag(s_subtitles_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_VOICE:
    lv_obj_remove_flag(s_voice_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_SYSTEM:
  case ECHOEAR_SETTINGS_PAGE_RESET_CONFIRM:
    lv_label_set_text(s_reset_label, "Factory reset");
    lv_obj_set_style_text_color(s_reset_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_remove_flag(s_system_page, LV_OBJ_FLAG_HIDDEN);
    break;
  case ECHOEAR_SETTINGS_PAGE_ROOT:
  default:
    lv_obj_remove_flag(s_root_page, LV_OBJ_FLAG_HIDDEN);
    break;
  }
}

void echoear_settings_ui_create(lv_obj_t *parent) {
  if (echoear_settings_get()->generation == 0U) {
    echoear_settings_init(NULL);
  }

  s_panel = lv_obj_create(parent);
  lv_obj_remove_style_all(s_panel);
  lv_obj_set_size(s_panel, 360, 360);
  lv_obj_center(s_panel);
  lv_obj_set_style_radius(s_panel, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(s_panel, lv_color_hex(COLOR_PANEL), 0);
  lv_obj_set_style_bg_opa(s_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_clip_corner(s_panel, true, 0);
  lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);

  s_root_page = make_page(s_panel, "SETTINGS");
  lv_obj_t *hint = make_label(s_root_page, "Hold face 5s to open again");
  lv_obj_set_style_text_color(hint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 48);

  lv_obj_t *list = lv_obj_create(s_root_page);
  lv_obj_remove_style_all(list);
  lv_obj_set_size(list, 260, 176);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 68);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(list, 7, 0);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);

  make_root_row(list, "Wi-Fi setup", "OPEN", wifi_clicked, NULL, NULL);
  make_root_row(list, "Brightness", "", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_DISPLAY,
                &s_root_brightness_value);
  make_root_row(list, "Volume", "", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_AUDIO,
                &s_root_volume_value);
  make_root_row(list, "Car Mode", "", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_CAR_MODE,
                &s_root_car_value);
  make_root_row(list, "Hands-free", "", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_HANDS_FREE,
                &s_root_hands_free_value);
  make_root_row(list, "Battery", "", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_BATTERY,
                &s_root_battery_value);
  make_root_row(list, "Speech text", "", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_SUBTITLES,
                &s_root_subtitles_value);
  make_root_row(list, "Voice", "", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_VOICE,
                &s_root_voice_value);
  make_root_row(list, "System", ">", page_clicked,
                (void *)(uintptr_t)ECHOEAR_SETTINGS_PAGE_SYSTEM, NULL);

  make_button(s_root_page, "CLOSE", 62, 250, 104, 42, cancel_clicked, NULL,
              NULL);
  make_button(s_root_page, "SAVE", 194, 250, 104, 42, save_clicked, NULL, NULL);

  s_display_page = make_page(s_panel, "BRIGHTNESS");
  s_brightness_value = make_label(s_display_page, "");
  lv_obj_set_style_text_color(s_brightness_value, lv_color_hex(COLOR_CYAN), 0);
  lv_obj_align(s_brightness_value, LV_ALIGN_CENTER, 0, -52);
  s_brightness = lv_slider_create(s_display_page);
  lv_obj_set_size(s_brightness, 224, 18);
  lv_obj_align(s_brightness, LV_ALIGN_CENTER, 0, 0);
  lv_slider_set_range(s_brightness, 0, 100);
  style_slider(s_brightness);
  lv_obj_add_event_cb(s_brightness, brightness_changed, LV_EVENT_VALUE_CHANGED,
                      NULL);
  make_detail_footer(s_display_page);

  s_audio_page = make_page(s_panel, "VOLUME");
  s_volume_value = make_label(s_audio_page, "");
  lv_obj_set_style_text_color(s_volume_value, lv_color_hex(COLOR_CYAN), 0);
  lv_obj_align(s_volume_value, LV_ALIGN_CENTER, 0, -52);
  s_volume = lv_slider_create(s_audio_page);
  lv_obj_set_size(s_volume, 224, 18);
  lv_obj_align(s_volume, LV_ALIGN_CENTER, 0, 0);
  lv_slider_set_range(s_volume, 0, 100);
  style_slider(s_volume);
  lv_obj_add_event_cb(s_volume, volume_changed, LV_EVENT_VALUE_CHANGED, NULL);
  make_detail_footer(s_audio_page);

  s_car_page = make_page(s_panel, "CAR MODE");
  lv_obj_t *car_hint = make_label(s_car_page, "Vehicle information and alerts");
  lv_obj_set_style_text_color(car_hint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(car_hint, LV_ALIGN_CENTER, 0, -46);
  s_car_mode = lv_switch_create(s_car_page);
  lv_obj_align(s_car_mode, LV_ALIGN_CENTER, 0, 14);
  lv_obj_add_event_cb(s_car_mode, car_mode_changed, LV_EVENT_VALUE_CHANGED,
                      NULL);
  make_detail_footer(s_car_page);

  s_hands_free_page = make_page(s_panel, "HANDS-FREE");
  lv_obj_t *hands_free_hint =
      make_label(s_hands_free_page, "Talk without tapping the screen");
  lv_obj_set_style_text_color(hands_free_hint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(hands_free_hint, LV_ALIGN_CENTER, 0, -46);
  s_hands_free = lv_switch_create(s_hands_free_page);
  lv_obj_align(s_hands_free, LV_ALIGN_CENTER, 0, 14);
  lv_obj_add_event_cb(s_hands_free, hands_free_changed, LV_EVENT_VALUE_CHANGED,
                      NULL);
  make_detail_footer(s_hands_free_page);

  s_battery_page = make_page(s_panel, "BATTERY");
  s_battery_level_value = make_label(s_battery_page, "Battery --");
  lv_obj_set_style_text_color(s_battery_level_value, lv_color_hex(COLOR_CYAN), 0);
  lv_obj_align(s_battery_level_value, LV_ALIGN_CENTER, 0, -52);
  lv_obj_t *battery_hint = make_label(s_battery_page, "Show status on face");
  lv_obj_set_style_text_color(battery_hint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(battery_hint, LV_ALIGN_CENTER, 0, -12);
  s_show_battery = lv_switch_create(s_battery_page);
  lv_obj_align(s_show_battery, LV_ALIGN_CENTER, 0, 32);
  lv_obj_add_event_cb(s_show_battery, show_battery_changed,
                      LV_EVENT_VALUE_CHANGED, NULL);
  make_detail_footer(s_battery_page);

  s_subtitles_page = make_page(s_panel, "SPEECH TEXT");
  lv_obj_t *subtitles_hint =
      make_label(s_subtitles_page, "Show spoken replies at the bottom");
  lv_obj_set_width(subtitles_hint, 260);
  lv_obj_set_style_text_align(subtitles_hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(subtitles_hint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(subtitles_hint, LV_ALIGN_CENTER, 0, -46);
  s_show_subtitles = lv_switch_create(s_subtitles_page);
  lv_obj_align(s_show_subtitles, LV_ALIGN_CENTER, 0, 24);
  lv_obj_add_event_cb(s_show_subtitles, show_subtitles_changed,
                      LV_EVENT_VALUE_CHANGED, NULL);
  make_detail_footer(s_subtitles_page);

  s_voice_page = make_page(s_panel, "ASSISTANT VOICE");
  lv_obj_t *voice_hint = make_label(s_voice_page, "Choose a voice style");
  lv_obj_set_style_text_color(voice_hint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(voice_hint, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_t *voice_list = lv_obj_create(s_voice_page);
  lv_obj_remove_style_all(voice_list);
  lv_obj_set_size(voice_list, 260, 174);
  lv_obj_align(voice_list, LV_ALIGN_TOP_MID, 0, 70);
  lv_obj_set_flex_flow(voice_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(voice_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(voice_list, 7, 0);
  lv_obj_set_scroll_dir(voice_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(voice_list, LV_SCROLLBAR_MODE_ACTIVE);
  make_root_row(voice_list, "Cute & bright", "", voice_changed,
                (void *)(uintptr_t)ECHOEAR_ASSISTANT_VOICE_LEDA,
                &s_voice_option_values[ECHOEAR_ASSISTANT_VOICE_LEDA]);
  make_root_row(voice_list, "Warm", "", voice_changed,
                (void *)(uintptr_t)ECHOEAR_ASSISTANT_VOICE_SULAFAT,
                &s_voice_option_values[ECHOEAR_ASSISTANT_VOICE_SULAFAT]);
  make_root_row(voice_list, "Soft & breezy", "", voice_changed,
                (void *)(uintptr_t)ECHOEAR_ASSISTANT_VOICE_AOEDE,
                &s_voice_option_values[ECHOEAR_ASSISTANT_VOICE_AOEDE]);
  make_root_row(voice_list, "Friendly", "", voice_changed,
                (void *)(uintptr_t)ECHOEAR_ASSISTANT_VOICE_ACHIRD,
                &s_voice_option_values[ECHOEAR_ASSISTANT_VOICE_ACHIRD]);
  make_root_row(voice_list, "Firm", "", voice_changed,
                (void *)(uintptr_t)ECHOEAR_ASSISTANT_VOICE_KORE,
                &s_voice_option_values[ECHOEAR_ASSISTANT_VOICE_KORE]);
  make_detail_footer(s_voice_page);

  s_system_page = make_page(s_panel, "SYSTEM");
  make_button(s_system_page, "Factory reset", 80, 132, 200, 52, reset_clicked,
              NULL, &s_reset_label);
  make_button(s_system_page, "BACK", 128, 250, 104, 42, back_clicked, NULL,
              NULL);

  s_save_feedback = lv_obj_create(s_panel);
  lv_obj_remove_style_all(s_save_feedback);
  lv_obj_set_size(s_save_feedback, 150, 70);
  lv_obj_center(s_save_feedback);
  lv_obj_set_style_radius(s_save_feedback, 24, 0);
  lv_obj_set_style_bg_color(s_save_feedback, lv_color_hex(0x43F5E8), 0);
  lv_obj_set_style_bg_opa(s_save_feedback, LV_OPA_COVER, 0);
  lv_obj_remove_flag(s_save_feedback, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *saved_label = make_label(s_save_feedback, "SAVED");
  lv_obj_center(saved_label);
  lv_obj_set_style_text_color(saved_label, lv_color_hex(COLOR_BLACK), 0);
  lv_obj_add_flag(s_save_feedback, LV_OBJ_FLAG_HIDDEN);
  s_save_timer =
      lv_timer_create(save_feedback_timer, SETTINGS_SAVE_FEEDBACK_MS, NULL);
  lv_timer_pause(s_save_timer);

  lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
  show_page(ECHOEAR_SETTINGS_PAGE_ROOT);

  s_hold_overlay = lv_obj_create(parent);
  lv_obj_remove_style_all(s_hold_overlay);
  lv_obj_set_size(s_hold_overlay, 190, 68);
  lv_obj_align(s_hold_overlay, LV_ALIGN_BOTTOM_MID, 0, -34);
  lv_obj_set_style_radius(s_hold_overlay, 20, 0);
  lv_obj_set_style_bg_color(s_hold_overlay, lv_color_hex(COLOR_BLACK), 0);
  lv_obj_set_style_bg_opa(s_hold_overlay, LV_OPA_80, 0);
  lv_obj_set_style_border_color(s_hold_overlay, lv_color_hex(COLOR_CYAN), 0);
  lv_obj_set_style_border_width(s_hold_overlay, 2, 0);
  lv_obj_remove_flag(s_hold_overlay, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *hold_label =
      make_label(s_hold_overlay, "Keep holding for Settings");
  lv_obj_set_width(hold_label, 150);
  lv_obj_align(hold_label, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_text_color(hold_label, lv_color_hex(0x43F5E8), 0);
  lv_obj_set_style_text_align(hold_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(hold_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_anim_duration(hold_label, 3200, 0);

  s_hold_bar = lv_bar_create(s_hold_overlay);
  lv_obj_set_size(s_hold_bar, 150, 8);
  lv_obj_align(s_hold_bar, LV_ALIGN_BOTTOM_MID, 0, -12);
  lv_bar_set_range(s_hold_bar, 0, 100);
  lv_bar_set_value(s_hold_bar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(s_hold_bar, lv_color_hex(0x24414A), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_hold_bar, lv_color_hex(COLOR_CYAN),
                            LV_PART_INDICATOR);
  lv_obj_add_flag(s_hold_overlay, LV_OBJ_FLAG_HIDDEN);

  s_hold_timer = lv_timer_create(settings_hold_timer, 100, NULL);
  lv_timer_pause(s_hold_timer);
  lv_obj_add_event_cb(parent, settings_hold_event, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(parent, settings_hold_event, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(parent, settings_hold_event, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(parent, settings_hold_event, LV_EVENT_PRESS_LOST, NULL);
}

void echoear_settings_ui_show(void) {
  echoear_settings_open();
  echoear_settings_ui_refresh();
  show_page(ECHOEAR_SETTINGS_PAGE_ROOT);
  s_panel_visible = true;
  lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_panel);
  if (s_visibility_handler != NULL) {
    s_visibility_handler(true);
  }
}

void echoear_settings_ui_set_short_tap_handler(
    echoear_settings_ui_short_tap_handler_t handler) {
  s_short_tap_handler = handler;
}

void echoear_settings_ui_set_visibility_handler(
    echoear_settings_ui_visibility_handler_t handler) {
  s_visibility_handler = handler;
}

void echoear_settings_ui_hide(void) {
  s_panel_visible = false;
  echoear_settings_close();
  if (s_save_timer != NULL) {
    lv_timer_pause(s_save_timer);
  }
  if (s_save_feedback != NULL) {
    lv_obj_add_flag(s_save_feedback, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
  if (s_visibility_handler != NULL) {
    s_visibility_handler(false);
  }
}

bool echoear_settings_ui_is_visible(void) { return s_panel_visible; }

void echoear_settings_ui_refresh(void) {
  const echoear_settings_t *settings = echoear_settings_get();
  lv_slider_set_value(s_brightness, settings->brightness, LV_ANIM_OFF);
  lv_slider_set_value(s_volume, settings->volume, LV_ANIM_OFF);
  if (settings->car_mode) {
    lv_obj_add_state(s_car_mode, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(s_car_mode, LV_STATE_CHECKED);
  }
  if (settings->hands_free) {
    lv_obj_add_state(s_hands_free, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(s_hands_free, LV_STATE_CHECKED);
  }
  if (settings->show_battery) {
    lv_obj_add_state(s_show_battery, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(s_show_battery, LV_STATE_CHECKED);
  }
  if (settings->show_subtitles) {
    lv_obj_add_state(s_show_subtitles, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(s_show_subtitles, LV_STATE_CHECKED);
  }
  refresh_value_labels();
}

void echoear_settings_ui_set_battery_status(int level, bool charging,
                                            bool valid) {
  if (level < 0) {
    level = 0;
  } else if (level > 100) {
    level = 100;
  }
  s_battery_level = level;
  s_battery_charging = charging;
  s_battery_valid = valid;
  if (s_battery_level_value != NULL) {
    char value[20];
    if (!valid) {
      lv_label_set_text(s_battery_level_value, "Battery --");
    } else {
      snprintf(value, sizeof(value), charging ? "%d%% charging" : "%d%%",
               level);
      lv_label_set_text(s_battery_level_value, value);
    }
  }
}
