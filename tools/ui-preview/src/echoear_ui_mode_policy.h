#pragma once

#include "echoear_app_state.h"

#include <stdbool.h>

typedef enum {
    ECHOEAR_UI_MODE_INTERACTION = 0,
    ECHOEAR_UI_MODE_VEHICLE
} echoear_ui_mode_t;

bool echoear_ui_interaction_is_active(echoear_interaction_state_t state);
bool echoear_ui_vehicle_alert_is_critical(echoear_vehicle_scenario_t scenario);
echoear_ui_mode_t echoear_ui_resolve_mode(const echoear_app_state_t *state);
