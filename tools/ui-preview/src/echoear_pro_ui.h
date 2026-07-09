#pragma once

#include "lvgl/lvgl.h"

typedef enum {
    ECHOEAR_STATE_IDLE = 0,
    ECHOEAR_STATE_LISTENING,
    ECHOEAR_STATE_THINKING,
    ECHOEAR_STATE_SPEAKING,
    ECHOEAR_STATE_SLEEPING,
    ECHOEAR_STATE_CAR_MODE
} echoear_state_t;

void echoear_pro_ui_create(void);
void echoear_pro_ui_set_state(echoear_state_t state);