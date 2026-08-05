#pragma once

#include "lvgl/lvgl.h"

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
void echoear_pro_ui_refresh(void);
