#pragma once

#include <stdbool.h>

#if defined(ESP_PLATFORM)
#include <lvgl.h>
#else
#include "lvgl/lvgl.h"
#endif

typedef enum
{
    ECHOEAR_FACE_NORMAL_IDLE = 0,
    ECHOEAR_FACE_NORMAL_ANGRY,
    ECHOEAR_FACE_NORMAL_CONFUSED,
    ECHOEAR_FACE_NORMAL_HAPPY,
    ECHOEAR_FACE_NORMAL_LISTENING,
    ECHOEAR_FACE_NORMAL_SAD,
    ECHOEAR_FACE_NORMAL_SLEEPING,
    ECHOEAR_FACE_NORMAL_SPEAKING,
    ECHOEAR_FACE_NORMAL_SURPRISED,
    ECHOEAR_FACE_NORMAL_THINKING,
    ECHOEAR_FACE_NORMAL_WINK,

    ECHOEAR_FACE_SYSTEM_WIFI_SETUP,
    ECHOEAR_FACE_SYSTEM_OTA_UPDATING,
    ECHOEAR_FACE_SYSTEM_LOW_BATTERY,
    ECHOEAR_FACE_SYSTEM_ERROR,

    ECHOEAR_FACE_CAR_OBD_CONNECTING,
    ECHOEAR_FACE_CAR_OBD_READY,
    ECHOEAR_FACE_CAR_OBD_ERROR,

    /* Module 3B dedicated vehicle face assets. */
    ECHOEAR_FACE_CAR_PARKED,
    ECHOEAR_FACE_CAR_CHARGING,
    ECHOEAR_FACE_CAR_LOW_BATTERY,
    ECHOEAR_FACE_CAR_DOOR_OPEN,
    ECHOEAR_FACE_CAR_CLOUD_STALE,
    ECHOEAR_FACE_CAR_CLOUD_OFFLINE,
    ECHOEAR_FACE_CAR_DRIVING
} echoear_face_state_t;

void echoear_pro_ui_create(void);
void echoear_pro_ui_set_state(echoear_face_state_t state);
void echoear_pro_ui_apply_interaction_state(void);
void echoear_pro_ui_apply_vehicle_state(void);
void echoear_pro_ui_apply_app_state(void);
void echoear_pro_ui_apply_provisioning_state(void);
void echoear_pro_ui_refresh(void);
void echoear_pro_ui_set_subtitle(const char *text);
void echoear_pro_ui_clear_subtitle_after(uint32_t delay_ms);
void echoear_pro_ui_set_device_battery(int level, bool charging, bool valid);
void echoear_pro_ui_set_battery_visible(bool visible);
