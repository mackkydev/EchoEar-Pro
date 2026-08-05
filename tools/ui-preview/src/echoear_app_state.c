#include "echoear_app_state.h"

#include <stddef.h>
#include <string.h>

static echoear_app_state_t app_state;

static void copy_text(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static int clamp_int(int value, int minimum, int maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static float clamp_float(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

void echoear_app_state_init(void)
{
    memset(&app_state, 0, sizeof(app_state));

    copy_text(app_state.device_id, sizeof(app_state.device_id), "EEPRO-0001");
    copy_text(app_state.device_name, sizeof(app_state.device_name), "EchoEar Pro");
    copy_text(app_state.assistant_name, sizeof(app_state.assistant_name), "Ava");
    copy_text(app_state.owner_name, sizeof(app_state.owner_name), "Owner");
    copy_text(app_state.wake_word, sizeof(app_state.wake_word), "Hey EchoEar");
    copy_text(app_state.language, sizeof(app_state.language), "th-TH");

    app_state.brightness = 90;
    app_state.volume = 70;
    app_state.animation_speed = 1.0f;
    app_state.car_mode = false;
    app_state.interaction_state = ECHOEAR_INTERACTION_IDLE;

    copy_text(app_state.vehicle.status, sizeof(app_state.vehicle.status), "VEHICLE READY");
    app_state.vehicle.scenario = ECHOEAR_VEHICLE_SCENARIO_PARKED;
    app_state.vehicle.availability = ECHOEAR_VEHICLE_AVAILABILITY_ONLINE;
    app_state.vehicle.speed_source = ECHOEAR_SPEED_SOURCE_UNAVAILABLE;
    app_state.vehicle.soc_percent = 82;
    app_state.vehicle.range_km = 478;
    app_state.vehicle.speed_kph = 0.0f;
    app_state.vehicle.plugged_in = false;
    app_state.vehicle.charging = false;
    app_state.vehicle.charge_power_kw = 0.0f;
    app_state.vehicle.charge_limit_percent = 80;
    app_state.vehicle.locked = true;
    app_state.vehicle.doors_open = false;
    app_state.vehicle.cabin_temperature_c = 31.5f;
}

echoear_app_state_t *echoear_app_state_get(void)
{
    return &app_state;
}

void echoear_app_state_set_device_name(const char *name)
{
    copy_text(app_state.device_name, sizeof(app_state.device_name), name);
}

void echoear_app_state_set_assistant_name(const char *name)
{
    copy_text(app_state.assistant_name, sizeof(app_state.assistant_name), name);
}

void echoear_app_state_set_owner_name(const char *name)
{
    copy_text(app_state.owner_name, sizeof(app_state.owner_name), name);
}

void echoear_app_state_set_brightness(uint8_t brightness)
{
    if (brightness > 100) {
        brightness = 100;
    }
    app_state.brightness = brightness;
}

void echoear_app_state_set_volume(uint8_t volume)
{
    if (volume > 100) {
        volume = 100;
    }
    app_state.volume = volume;
}

void echoear_app_state_set_animation_speed(float speed)
{
    app_state.animation_speed = clamp_float(speed, 0.25f, 3.0f);
}

void echoear_app_state_set_car_mode(bool enabled)
{
    app_state.car_mode = enabled;
}

void echoear_app_state_set_interaction_state(echoear_interaction_state_t state)
{
    if (state < ECHOEAR_INTERACTION_IDLE || state > ECHOEAR_INTERACTION_SURPRISED) {
        state = ECHOEAR_INTERACTION_IDLE;
    }
    app_state.interaction_state = state;
}

void echoear_app_state_set_vehicle_status(const char *status)
{
    copy_text(app_state.vehicle.status, sizeof(app_state.vehicle.status), status);
}

void echoear_app_state_set_vehicle_scenario(echoear_vehicle_scenario_t scenario)
{
    app_state.vehicle.scenario = scenario;
}

void echoear_app_state_set_vehicle_availability(echoear_vehicle_availability_t availability)
{
    app_state.vehicle.availability = availability;
}

void echoear_app_state_set_speed_source(echoear_speed_source_t source)
{
    app_state.vehicle.speed_source = source;
}

void echoear_app_state_set_vehicle_metrics(int soc_percent, int range_km, float speed_kph)
{
    app_state.vehicle.soc_percent = clamp_int(soc_percent, 0, 100);
    app_state.vehicle.range_km = range_km < 0 ? 0 : range_km;
    app_state.vehicle.speed_kph = speed_kph < 0.0f ? 0.0f : speed_kph;
}

void echoear_app_state_set_vehicle_charging(
    bool plugged_in,
    bool charging,
    float charge_power_kw,
    int charge_limit_percent
)
{
    app_state.vehicle.plugged_in = plugged_in;
    app_state.vehicle.charging = charging;
    app_state.vehicle.charge_power_kw = charge_power_kw < 0.0f ? 0.0f : charge_power_kw;
    app_state.vehicle.charge_limit_percent = clamp_int(charge_limit_percent, 0, 100);
}

void echoear_app_state_set_vehicle_security(bool locked, bool doors_open)
{
    app_state.vehicle.locked = locked;
    app_state.vehicle.doors_open = doors_open;
}

void echoear_app_state_set_cabin_temperature(float cabin_temperature_c)
{
    app_state.vehicle.cabin_temperature_c = cabin_temperature_c;
}

void echoear_app_state_set_obd(
    const char *status,
    int soc_percent,
    int range_km,
    int speed_kmh
)
{
    echoear_app_state_set_vehicle_status(status);
    echoear_app_state_set_vehicle_metrics(soc_percent, range_km, (float)speed_kmh);
    echoear_app_state_set_speed_source(ECHOEAR_SPEED_SOURCE_OBD);
}


const char *echoear_interaction_state_to_string(echoear_interaction_state_t state)
{
    switch (state) {
    case ECHOEAR_INTERACTION_LISTENING:
        return "listening";
    case ECHOEAR_INTERACTION_THINKING:
        return "thinking";
    case ECHOEAR_INTERACTION_SPEAKING:
        return "speaking";
    case ECHOEAR_INTERACTION_HAPPY:
        return "happy";
    case ECHOEAR_INTERACTION_CONFUSED:
        return "confused";
    case ECHOEAR_INTERACTION_SAD:
        return "sad";
    case ECHOEAR_INTERACTION_SLEEPING:
        return "sleeping";
    case ECHOEAR_INTERACTION_WINK:
        return "wink";
    case ECHOEAR_INTERACTION_ANGRY:
        return "angry";
    case ECHOEAR_INTERACTION_SURPRISED:
        return "surprised";
    case ECHOEAR_INTERACTION_IDLE:
    default:
        return "idle";
    }
}

const char *echoear_vehicle_scenario_to_string(echoear_vehicle_scenario_t scenario)
{
    switch (scenario) {
    case ECHOEAR_VEHICLE_SCENARIO_CHARGING:
        return "charging";
    case ECHOEAR_VEHICLE_SCENARIO_LOW_BATTERY:
        return "low_battery";
    case ECHOEAR_VEHICLE_SCENARIO_DOOR_OPEN:
        return "door_open";
    case ECHOEAR_VEHICLE_SCENARIO_CLOUD_STALE:
        return "cloud_stale";
    case ECHOEAR_VEHICLE_SCENARIO_CLOUD_OFFLINE:
        return "cloud_offline";
    case ECHOEAR_VEHICLE_SCENARIO_DRIVING_GPS:
        return "driving_gps";
    case ECHOEAR_VEHICLE_SCENARIO_PARKED:
    default:
        return "parked";
    }
}

const char *echoear_vehicle_availability_to_string(echoear_vehicle_availability_t availability)
{
    switch (availability) {
    case ECHOEAR_VEHICLE_AVAILABILITY_STALE:
        return "stale";
    case ECHOEAR_VEHICLE_AVAILABILITY_OFFLINE:
        return "offline";
    case ECHOEAR_VEHICLE_AVAILABILITY_ONLINE:
    default:
        return "online";
    }
}

const char *echoear_speed_source_to_string(echoear_speed_source_t source)
{
    switch (source) {
    case ECHOEAR_SPEED_SOURCE_OBD:
        return "obd";
    case ECHOEAR_SPEED_SOURCE_PHONE_GPS:
        return "phone_gps";
    case ECHOEAR_SPEED_SOURCE_VEHICLE:
        return "vehicle";
    case ECHOEAR_SPEED_SOURCE_UNAVAILABLE:
    default:
        return "unavailable";
    }
}
