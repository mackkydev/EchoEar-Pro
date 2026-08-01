#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char status[32];
    int soc_percent;
    int range_km;
    int speed_kmh;
} echoear_obd_state_t;

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
    echoear_obd_state_t obd;
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

void echoear_app_state_set_obd(
    const char *status,
    int soc_percent,
    int range_km,
    int speed_kmh
);