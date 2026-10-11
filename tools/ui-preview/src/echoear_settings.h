#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "echoear_app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECHOEAR_SETTINGS_PAGE_ROOT = 0,
    ECHOEAR_SETTINGS_PAGE_DISPLAY,
    ECHOEAR_SETTINGS_PAGE_AUDIO,
    ECHOEAR_SETTINGS_PAGE_CAR_MODE,
    ECHOEAR_SETTINGS_PAGE_HANDS_FREE,
    ECHOEAR_SETTINGS_PAGE_BATTERY,
    ECHOEAR_SETTINGS_PAGE_SUBTITLES,
    ECHOEAR_SETTINGS_PAGE_VOICE,
    ECHOEAR_SETTINGS_PAGE_SYSTEM,
    ECHOEAR_SETTINGS_PAGE_RESET_CONFIRM
} echoear_settings_page_t;

typedef struct {
    bool (*request_wifi_setup)(void);
    bool (*request_factory_reset)(void);
    bool (*load)(uint8_t *brightness, uint8_t *volume,
                 float *animation_speed, bool *car_mode, bool *hands_free,
                 bool *show_battery, bool *show_subtitles,
                 echoear_assistant_voice_t *assistant_voice);
    bool (*save)(uint8_t brightness, uint8_t volume,
                 float animation_speed, bool car_mode, bool hands_free,
                 bool show_battery, bool show_subtitles,
                 echoear_assistant_voice_t assistant_voice);
} echoear_settings_ops_t;

typedef struct {
    bool open;
    bool dirty;
    echoear_settings_page_t page;
    uint8_t brightness;
    uint8_t volume;
    float animation_speed;
    bool car_mode;
    bool hands_free;
    bool show_battery;
    bool show_subtitles;
    echoear_assistant_voice_t assistant_voice;
    uint32_t saves;
    uint32_t cancels;
    uint32_t failures;
    uint32_t generation;
} echoear_settings_t;

void echoear_settings_init(const echoear_settings_ops_t *ops);
echoear_settings_t *echoear_settings_get(void);
void echoear_settings_open(void);
void echoear_settings_close(void);
bool echoear_settings_navigate(echoear_settings_page_t page);
void echoear_settings_set_brightness(uint8_t brightness);
void echoear_settings_set_volume(uint8_t volume);
void echoear_settings_set_animation_speed(float speed);
void echoear_settings_set_car_mode(bool enabled);
void echoear_settings_set_hands_free(bool enabled);
void echoear_settings_set_show_battery(bool enabled);
void echoear_settings_set_show_subtitles(bool enabled);
void echoear_settings_set_assistant_voice(echoear_assistant_voice_t voice);
bool echoear_settings_save(void);
void echoear_settings_cancel(void);
bool echoear_settings_request_wifi_setup(void);
bool echoear_settings_request_factory_reset(void);
const char *echoear_settings_page_name(echoear_settings_page_t page);
const char *echoear_assistant_voice_name(echoear_assistant_voice_t voice);
const char *echoear_assistant_voice_label(echoear_assistant_voice_t voice);

#ifdef __cplusplus
}
#endif
