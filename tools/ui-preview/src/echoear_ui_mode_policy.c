#include "echoear_ui_mode_policy.h"

bool echoear_ui_interaction_is_active(echoear_interaction_state_t state)
{
    return state == ECHOEAR_INTERACTION_LISTENING ||
           state == ECHOEAR_INTERACTION_THINKING ||
           state == ECHOEAR_INTERACTION_SPEAKING;
}

bool echoear_ui_vehicle_alert_is_critical(echoear_vehicle_scenario_t scenario)
{
    return scenario == ECHOEAR_VEHICLE_SCENARIO_LOW_BATTERY ||
           scenario == ECHOEAR_VEHICLE_SCENARIO_DOOR_OPEN;
}

echoear_ui_mode_t echoear_ui_resolve_mode(const echoear_app_state_t *state)
{
    if (state == 0 || !state->car_mode) {
        return ECHOEAR_UI_MODE_INTERACTION;
    }
    if (echoear_ui_vehicle_alert_is_critical(state->vehicle.scenario)) {
        return ECHOEAR_UI_MODE_VEHICLE;
    }
    if (echoear_ui_interaction_is_active(state->interaction_state)) {
        return ECHOEAR_UI_MODE_INTERACTION;
    }
    return ECHOEAR_UI_MODE_VEHICLE;
}
