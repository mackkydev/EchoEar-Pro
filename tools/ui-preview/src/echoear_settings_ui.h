#pragma once

#if defined(ESP_PLATFORM)
#include <lvgl.h>
#else
#include "lvgl/lvgl.h"
#endif

typedef void (*echoear_settings_ui_short_tap_handler_t)(void);
typedef void (*echoear_settings_ui_visibility_handler_t)(bool visible);

void echoear_settings_ui_create(lv_obj_t *parent);
void echoear_settings_ui_set_short_tap_handler(
    echoear_settings_ui_short_tap_handler_t handler);
void echoear_settings_ui_set_visibility_handler(
    echoear_settings_ui_visibility_handler_t handler);
void echoear_settings_ui_show(void);
void echoear_settings_ui_hide(void);
bool echoear_settings_ui_is_visible(void);
void echoear_settings_ui_refresh(void);
void echoear_settings_ui_set_battery_status(int level, bool charging,
                                            bool valid);
