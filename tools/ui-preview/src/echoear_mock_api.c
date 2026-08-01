#include "echoear_mock_api.h"
#include "echoear_app_state.h"
#include "echoear_pro_ui.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim_whitespace(char *text)
{
    char *end;

    if (text == NULL) {
        return NULL;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }

    if (*text == '\0') {
        return text;
    }

    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) {
        end--;
    }
    end[1] = '\0';

    return text;
}

static bool parse_bool(const char *value)
{
    return strcmp(value, "1") == 0 ||
           strcmp(value, "true") == 0 ||
           strcmp(value, "yes") == 0 ||
           strcmp(value, "on") == 0;
}

static bool parse_face(const char *value, echoear_face_state_t *face)
{
    if (value == NULL || face == NULL) {
        return false;
    }

    if (strcmp(value, "normal_idle") == 0)
        *face = ECHOEAR_FACE_NORMAL_IDLE;
    else if (strcmp(value, "normal_angry") == 0)
        *face = ECHOEAR_FACE_NORMAL_ANGRY;
    else if (strcmp(value, "normal_confused") == 0)
        *face = ECHOEAR_FACE_NORMAL_CONFUSED;
    else if (strcmp(value, "normal_happy") == 0)
        *face = ECHOEAR_FACE_NORMAL_HAPPY;
    else if (strcmp(value, "normal_listening") == 0)
        *face = ECHOEAR_FACE_NORMAL_LISTENING;
    else if (strcmp(value, "normal_sad") == 0)
        *face = ECHOEAR_FACE_NORMAL_SAD;
    else if (strcmp(value, "normal_sleeping") == 0)
        *face = ECHOEAR_FACE_NORMAL_SLEEPING;
    else if (strcmp(value, "normal_speaking") == 0)
        *face = ECHOEAR_FACE_NORMAL_SPEAKING;
    else if (strcmp(value, "normal_surprised") == 0)
        *face = ECHOEAR_FACE_NORMAL_SURPRISED;
    else if (strcmp(value, "normal_thinking") == 0)
        *face = ECHOEAR_FACE_NORMAL_THINKING;
    else if (strcmp(value, "normal_wink") == 0)
        *face = ECHOEAR_FACE_NORMAL_WINK;
    else if (strcmp(value, "system_wifi_setup") == 0)
        *face = ECHOEAR_FACE_SYSTEM_WIFI_SETUP;
    else if (strcmp(value, "system_ota_updating") == 0)
        *face = ECHOEAR_FACE_SYSTEM_OTA_UPDATING;
    else if (strcmp(value, "system_low_battery") == 0)
        *face = ECHOEAR_FACE_SYSTEM_LOW_BATTERY;
    else if (strcmp(value, "system_error") == 0)
        *face = ECHOEAR_FACE_SYSTEM_ERROR;
    else if (strcmp(value, "car_obd_connecting") == 0)
        *face = ECHOEAR_FACE_CAR_OBD_CONNECTING;
    else if (strcmp(value, "car_obd_ready") == 0)
        *face = ECHOEAR_FACE_CAR_OBD_READY;
    else if (strcmp(value, "car_obd_error") == 0)
        *face = ECHOEAR_FACE_CAR_OBD_ERROR;
    else if (strcmp(value, "car_parked") == 0)
        *face = ECHOEAR_FACE_CAR_PARKED;
    else if (strcmp(value, "car_charging") == 0)
        *face = ECHOEAR_FACE_CAR_CHARGING;
    else if (strcmp(value, "car_low_battery") == 0)
        *face = ECHOEAR_FACE_CAR_LOW_BATTERY;
    else if (strcmp(value, "car_door_open") == 0)
        *face = ECHOEAR_FACE_CAR_DOOR_OPEN;
    else if (strcmp(value, "car_cloud_stale") == 0)
        *face = ECHOEAR_FACE_CAR_CLOUD_STALE;
    else if (strcmp(value, "car_cloud_offline") == 0)
        *face = ECHOEAR_FACE_CAR_CLOUD_OFFLINE;
    else if (strcmp(value, "car_driving") == 0)
        *face = ECHOEAR_FACE_CAR_DRIVING;
    else
        return false;

    return true;
}

static echoear_vehicle_scenario_t parse_scenario(const char *value)
{
    if (strcmp(value, "charging") == 0)
        return ECHOEAR_VEHICLE_SCENARIO_CHARGING;
    if (strcmp(value, "low_battery") == 0)
        return ECHOEAR_VEHICLE_SCENARIO_LOW_BATTERY;
    if (strcmp(value, "door_open") == 0)
        return ECHOEAR_VEHICLE_SCENARIO_DOOR_OPEN;
    if (strcmp(value, "cloud_stale") == 0)
        return ECHOEAR_VEHICLE_SCENARIO_CLOUD_STALE;
    if (strcmp(value, "cloud_offline") == 0)
        return ECHOEAR_VEHICLE_SCENARIO_CLOUD_OFFLINE;
    if (strcmp(value, "driving_gps") == 0)
        return ECHOEAR_VEHICLE_SCENARIO_DRIVING_GPS;
    return ECHOEAR_VEHICLE_SCENARIO_PARKED;
}

static echoear_vehicle_availability_t parse_availability(const char *value)
{
    if (strcmp(value, "stale") == 0)
        return ECHOEAR_VEHICLE_AVAILABILITY_STALE;
    if (strcmp(value, "offline") == 0)
        return ECHOEAR_VEHICLE_AVAILABILITY_OFFLINE;
    return ECHOEAR_VEHICLE_AVAILABILITY_ONLINE;
}

static echoear_speed_source_t parse_speed_source(const char *value)
{
    if (strcmp(value, "obd") == 0)
        return ECHOEAR_SPEED_SOURCE_OBD;
    if (strcmp(value, "phone_gps") == 0)
        return ECHOEAR_SPEED_SOURCE_PHONE_GPS;
    if (strcmp(value, "vehicle") == 0)
        return ECHOEAR_SPEED_SOURCE_VEHICLE;
    return ECHOEAR_SPEED_SOURCE_UNAVAILABLE;
}

void echoear_mock_api_load(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return;
    }

    echoear_app_state_t *state = echoear_app_state_get();
    echoear_face_state_t requested_face = ECHOEAR_FACE_NORMAL_IDLE;
    bool has_face_override = false;
    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *key;
        char *value;
        char *equals;

        key = trim_whitespace(line);
        if (*key == '\0' || *key == '#' || *key == ';') {
            continue;
        }

        equals = strchr(key, '=');
        if (equals == NULL) {
            continue;
        }

        *equals = '\0';
        value = trim_whitespace(equals + 1);
        key = trim_whitespace(key);

        if (strcmp(key, "face") == 0) {
            if (strcmp(value, "auto") == 0 || *value == '\0') {
                has_face_override = false;
            }
            else if (parse_face(value, &requested_face)) {
                has_face_override = true;
            }
        }
        else if (strcmp(key, "device_name") == 0) {
            echoear_app_state_set_device_name(value);
        }
        else if (strcmp(key, "assistant_name") == 0) {
            echoear_app_state_set_assistant_name(value);
        }
        else if (strcmp(key, "owner_name") == 0) {
            echoear_app_state_set_owner_name(value);
        }
        else if (strcmp(key, "brightness") == 0) {
            echoear_app_state_set_brightness((uint8_t)atoi(value));
        }
        else if (strcmp(key, "volume") == 0) {
            echoear_app_state_set_volume((uint8_t)atoi(value));
        }
        else if (strcmp(key, "animation_speed") == 0) {
            echoear_app_state_set_animation_speed((float)atof(value));
        }
        else if (strcmp(key, "car_mode") == 0) {
            echoear_app_state_set_car_mode(parse_bool(value));
        }
        else if (strcmp(key, "scenario") == 0) {
            echoear_app_state_set_vehicle_scenario(parse_scenario(value));
        }
        else if (strcmp(key, "availability") == 0) {
            echoear_app_state_set_vehicle_availability(parse_availability(value));
        }
        else if (strcmp(key, "vehicle_status") == 0 || strcmp(key, "obd_status") == 0) {
            echoear_app_state_set_vehicle_status(value);
        }
        else if (strcmp(key, "speed_source") == 0) {
            echoear_app_state_set_speed_source(parse_speed_source(value));
        }
        else if (strcmp(key, "soc") == 0 || strcmp(key, "battery_percent") == 0) {
            echoear_app_state_set_vehicle_metrics(
                atoi(value),
                state->vehicle.range_km,
                state->vehicle.speed_kph);
        }
        else if (strcmp(key, "range") == 0 || strcmp(key, "range_km") == 0) {
            echoear_app_state_set_vehicle_metrics(
                state->vehicle.soc_percent,
                atoi(value),
                state->vehicle.speed_kph);
        }
        else if (strcmp(key, "speed") == 0 || strcmp(key, "speed_kph") == 0) {
            echoear_app_state_set_vehicle_metrics(
                state->vehicle.soc_percent,
                state->vehicle.range_km,
                (float)atof(value));
        }
        else if (strcmp(key, "plugged_in") == 0) {
            echoear_app_state_set_vehicle_charging(
                parse_bool(value),
                state->vehicle.charging,
                state->vehicle.charge_power_kw,
                state->vehicle.charge_limit_percent);
        }
        else if (strcmp(key, "charging") == 0) {
            echoear_app_state_set_vehicle_charging(
                state->vehicle.plugged_in,
                parse_bool(value),
                state->vehicle.charge_power_kw,
                state->vehicle.charge_limit_percent);
        }
        else if (strcmp(key, "charge_power_kw") == 0) {
            echoear_app_state_set_vehicle_charging(
                state->vehicle.plugged_in,
                state->vehicle.charging,
                (float)atof(value),
                state->vehicle.charge_limit_percent);
        }
        else if (strcmp(key, "charge_limit_percent") == 0) {
            echoear_app_state_set_vehicle_charging(
                state->vehicle.plugged_in,
                state->vehicle.charging,
                state->vehicle.charge_power_kw,
                atoi(value));
        }
        else if (strcmp(key, "locked") == 0) {
            echoear_app_state_set_vehicle_security(parse_bool(value), state->vehicle.doors_open);
        }
        else if (strcmp(key, "doors_open") == 0) {
            echoear_app_state_set_vehicle_security(state->vehicle.locked, parse_bool(value));
        }
        else if (strcmp(key, "cabin_temp_c") == 0 || strcmp(key, "cabin_temperature_c") == 0) {
            echoear_app_state_set_cabin_temperature((float)atof(value));
        }
    }

    fclose(fp);

    if (has_face_override) {
        echoear_pro_ui_set_state(requested_face);
    }
    else {
        echoear_pro_ui_apply_vehicle_state();
    }

    echoear_pro_ui_refresh();
}
