#include "echoear_mock_api.h"
#include "echoear_app_state.h"
#include "echoear_pro_ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void trim_newline(char *s)
{
    if (s == NULL)
        return;

    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    {
        s[len - 1] = '\0';
        len--;
    }
}

static echoear_face_state_t parse_face(const char *value)
{
    if (strcmp(value, "normal_idle") == 0)
        return ECHOEAR_FACE_NORMAL_IDLE;
    if (strcmp(value, "normal_angry") == 0)
        return ECHOEAR_FACE_NORMAL_ANGRY;
    if (strcmp(value, "normal_confused") == 0)
        return ECHOEAR_FACE_NORMAL_CONFUSED;
    if (strcmp(value, "normal_happy") == 0)
        return ECHOEAR_FACE_NORMAL_HAPPY;
    if (strcmp(value, "normal_listening") == 0)
        return ECHOEAR_FACE_NORMAL_LISTENING;
    if (strcmp(value, "normal_sad") == 0)
        return ECHOEAR_FACE_NORMAL_SAD;
    if (strcmp(value, "normal_sleeping") == 0)
        return ECHOEAR_FACE_NORMAL_SLEEPING;
    if (strcmp(value, "normal_speaking") == 0)
        return ECHOEAR_FACE_NORMAL_SPEAKING;
    if (strcmp(value, "normal_surprised") == 0)
        return ECHOEAR_FACE_NORMAL_SURPRISED;
    if (strcmp(value, "normal_thinking") == 0)
        return ECHOEAR_FACE_NORMAL_THINKING;
    if (strcmp(value, "normal_wink") == 0)
        return ECHOEAR_FACE_NORMAL_WINK;

    if (strcmp(value, "system_wifi_setup") == 0)
        return ECHOEAR_FACE_SYSTEM_WIFI_SETUP;
    if (strcmp(value, "system_ota_updating") == 0)
        return ECHOEAR_FACE_SYSTEM_OTA_UPDATING;
    if (strcmp(value, "system_low_battery") == 0)
        return ECHOEAR_FACE_SYSTEM_LOW_BATTERY;
    if (strcmp(value, "system_error") == 0)
        return ECHOEAR_FACE_SYSTEM_ERROR;

    if (strcmp(value, "car_obd_connecting") == 0)
        return ECHOEAR_FACE_CAR_OBD_CONNECTING;
    if (strcmp(value, "car_obd_ready") == 0)
        return ECHOEAR_FACE_CAR_OBD_READY;
    if (strcmp(value, "car_obd_error") == 0)
        return ECHOEAR_FACE_CAR_OBD_ERROR;

    return ECHOEAR_FACE_NORMAL_IDLE;
}

void echoear_mock_api_load(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        return;
    }

    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        trim_newline(line);

        char *eq = strchr(line, '=');
        if (eq == NULL)
            continue;

        *eq = '\0';
        const char *key = line;
        const char *value = eq + 1;

        if (strcmp(key, "face") == 0)
        {
            static char last_face[64] = "";

            if (strcmp(last_face, value) != 0)
            {
                strncpy(last_face, value, sizeof(last_face) - 1);
                last_face[sizeof(last_face) - 1] = '\0';

                echoear_pro_ui_set_state(parse_face(value));
            }
        }
        else if (strcmp(key, "device_name") == 0)
        {
            echoear_app_state_set_device_name(value);
        }
        else if (strcmp(key, "assistant_name") == 0)
        {
            echoear_app_state_set_assistant_name(value);
        }
        else if (strcmp(key, "owner_name") == 0)
        {
            echoear_app_state_set_owner_name(value);
        }
        else if (strcmp(key, "brightness") == 0)
        {
            echoear_app_state_set_brightness((uint8_t)atoi(value));
        }
        else if (strcmp(key, "volume") == 0)
        {
            echoear_app_state_set_volume((uint8_t)atoi(value));
        }
        else if (strcmp(key, "animation_speed") == 0)
        {
            echoear_app_state_set_animation_speed((float)atof(value));
        }
        else if (strcmp(key, "car_mode") == 0)
        {
            echoear_app_state_set_car_mode(atoi(value) != 0);
        }
        else if (strcmp(key, "obd_status") == 0)
        {
            echoear_app_state_t *state = echoear_app_state_get();
            echoear_app_state_set_obd(
                value,
                state->obd.soc_percent,
                state->obd.range_km,
                state->obd.speed_kmh);
        }
        else if (strcmp(key, "soc") == 0)
        {
            echoear_app_state_t *state = echoear_app_state_get();
            echoear_app_state_set_obd(
                state->obd.status,
                atoi(value),
                state->obd.range_km,
                state->obd.speed_kmh);
        }
        else if (strcmp(key, "range") == 0)
        {
            echoear_app_state_t *state = echoear_app_state_get();
            echoear_app_state_set_obd(
                state->obd.status,
                state->obd.soc_percent,
                atoi(value),
                state->obd.speed_kmh);
        }
        else if (strcmp(key, "speed") == 0)
        {
            echoear_app_state_t *state = echoear_app_state_get();
            echoear_app_state_set_obd(
                state->obd.status,
                state->obd.soc_percent,
                state->obd.range_km,
                atoi(value));
        }
    }

    fclose(fp);

    echoear_pro_ui_refresh();
}