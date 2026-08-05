#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ECHOEAR_VEHICLE_SCENARIO_PARKED = 0,
    ECHOEAR_VEHICLE_SCENARIO_CHARGING,
    ECHOEAR_VEHICLE_SCENARIO_LOW_BATTERY,
    ECHOEAR_VEHICLE_SCENARIO_DOOR_OPEN,
    ECHOEAR_VEHICLE_SCENARIO_CLOUD_STALE,
    ECHOEAR_VEHICLE_SCENARIO_CLOUD_OFFLINE,
    ECHOEAR_VEHICLE_SCENARIO_DRIVING_GPS
} echoear_vehicle_scenario_t;

typedef enum {
    ECHOEAR_VEHICLE_AVAILABILITY_ONLINE = 0,
    ECHOEAR_VEHICLE_AVAILABILITY_STALE,
    ECHOEAR_VEHICLE_AVAILABILITY_OFFLINE
} echoear_vehicle_availability_t;

typedef enum {
    ECHOEAR_SPEED_SOURCE_UNAVAILABLE = 0,
    ECHOEAR_SPEED_SOURCE_OBD,
    ECHOEAR_SPEED_SOURCE_PHONE_GPS,
    ECHOEAR_SPEED_SOURCE_VEHICLE
} echoear_speed_source_t;

typedef enum {
    ECHOEAR_INTERACTION_IDLE = 0,
    ECHOEAR_INTERACTION_LISTENING,
    ECHOEAR_INTERACTION_THINKING,
    ECHOEAR_INTERACTION_SPEAKING,
    ECHOEAR_INTERACTION_HAPPY,
    ECHOEAR_INTERACTION_CONFUSED,
    ECHOEAR_INTERACTION_SAD,
    ECHOEAR_INTERACTION_SLEEPING,
    ECHOEAR_INTERACTION_WINK,
    ECHOEAR_INTERACTION_ANGRY,
    ECHOEAR_INTERACTION_SURPRISED
} echoear_interaction_state_t;

typedef struct {
    char status[32];

    echoear_vehicle_scenario_t scenario;
    echoear_vehicle_availability_t availability;
    echoear_speed_source_t speed_source;

    int soc_percent;
    int range_km;
    float speed_kph;

    bool plugged_in;
    bool charging;
    float charge_power_kw;
    int charge_limit_percent;

    bool locked;
    bool doors_open;
    float cabin_temperature_c;
} echoear_vehicle_state_t;

typedef struct {
    char device_id[32];
    char device_name[64];
    char assistant_name[32];
    char owner_name[64];
    char wake_word[32];
    char language[16];

    uint8_t brightness;
    uint8_t volume;
    float animation_speed;

    bool car_mode;
    echoear_interaction_state_t interaction_state;
    echoear_vehicle_state_t vehicle;
} echoear_app_state_t;

void echoear_app_state_init(void);
echoear_app_state_t *echoear_app_state_get(void);

void echoear_app_state_set_device_name(const char *name);
void echoear_app_state_set_assistant_name(const char *name);
void echoear_app_state_set_owner_name(const char *name);
void echoear_app_state_set_brightness(uint8_t brightness);
void echoear_app_state_set_volume(uint8_t volume);
void echoear_app_state_set_animation_speed(float speed);
void echoear_app_state_set_car_mode(bool enabled);
void echoear_app_state_set_interaction_state(echoear_interaction_state_t state);

void echoear_app_state_set_vehicle_status(const char *status);
void echoear_app_state_set_vehicle_scenario(echoear_vehicle_scenario_t scenario);
void echoear_app_state_set_vehicle_availability(echoear_vehicle_availability_t availability);
void echoear_app_state_set_speed_source(echoear_speed_source_t source);
void echoear_app_state_set_vehicle_metrics(int soc_percent, int range_km, float speed_kph);
void echoear_app_state_set_vehicle_charging(
    bool plugged_in,
    bool charging,
    float charge_power_kw,
    int charge_limit_percent
);
void echoear_app_state_set_vehicle_security(bool locked, bool doors_open);
void echoear_app_state_set_cabin_temperature(float cabin_temperature_c);

/* Compatibility helper for the original OBD-only preview API. */
void echoear_app_state_set_obd(
    const char *status,
    int soc_percent,
    int range_km,
    int speed_kmh
);

const char *echoear_interaction_state_to_string(echoear_interaction_state_t state);
const char *echoear_vehicle_scenario_to_string(echoear_vehicle_scenario_t scenario);
const char *echoear_vehicle_availability_to_string(echoear_vehicle_availability_t availability);
const char *echoear_speed_source_to_string(echoear_speed_source_t source);
