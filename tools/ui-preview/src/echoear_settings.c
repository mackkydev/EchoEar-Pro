#include "echoear_settings.h"

#include "echoear_app_state.h"

#include <string.h>

static echoear_settings_t s_settings;
static echoear_settings_ops_t s_ops;

static void load_values(void)
{
    const echoear_app_state_t *app = echoear_app_state_get();

    s_settings.brightness = app->brightness;
    s_settings.volume = app->volume;
    s_settings.animation_speed = app->animation_speed;
    s_settings.car_mode = app->car_mode;
    s_settings.hands_free = app->hands_free;
    s_settings.show_battery = app->show_battery;
    s_settings.show_subtitles = app->show_subtitles;
    s_settings.assistant_voice = app->assistant_voice;
    s_settings.dirty = false;
    s_settings.generation++;
}

static void mark_dirty(void)
{
    s_settings.dirty = true;
    s_settings.generation++;
}

void echoear_settings_init(const echoear_settings_ops_t *ops)
{
    memset(&s_settings, 0, sizeof(s_settings));
    memset(&s_ops, 0, sizeof(s_ops));
    if (ops != NULL) {
        s_ops = *ops;
    }
    if (s_ops.load != NULL) {
        uint8_t brightness;
        uint8_t volume;
        float animation_speed;
        bool car_mode;
        bool hands_free;
        bool show_battery;
        bool show_subtitles;
        echoear_assistant_voice_t assistant_voice;
        if (s_ops.load(&brightness, &volume, &animation_speed, &car_mode,
                       &hands_free, &show_battery, &show_subtitles,
                       &assistant_voice)) {
            echoear_app_state_set_brightness(brightness);
            echoear_app_state_set_volume(volume);
            echoear_app_state_set_animation_speed(animation_speed);
            echoear_app_state_set_car_mode(car_mode);
            echoear_app_state_set_hands_free(hands_free);
            echoear_app_state_set_show_battery(show_battery);
            echoear_app_state_set_show_subtitles(show_subtitles);
            echoear_app_state_set_assistant_voice(assistant_voice);
        }
    }
    s_settings.page = ECHOEAR_SETTINGS_PAGE_ROOT;
    s_settings.generation = 1U;
    load_values();
}

echoear_settings_t *echoear_settings_get(void)
{
    return &s_settings;
}

void echoear_settings_open(void)
{
    load_values();
    s_settings.open = true;
    s_settings.page = ECHOEAR_SETTINGS_PAGE_ROOT;
    s_settings.generation++;
}

void echoear_settings_close(void)
{
    if (!s_settings.open) {
        return;
    }
    s_settings.open = false;
    s_settings.page = ECHOEAR_SETTINGS_PAGE_ROOT;
    s_settings.generation++;
}

bool echoear_settings_navigate(echoear_settings_page_t page)
{
    if (!s_settings.open || page < ECHOEAR_SETTINGS_PAGE_ROOT ||
        page > ECHOEAR_SETTINGS_PAGE_RESET_CONFIRM) {
        return false;
    }
    if (s_settings.page != page) {
        s_settings.page = page;
        s_settings.generation++;
    }
    return true;
}

void echoear_settings_set_brightness(uint8_t brightness)
{
    if (brightness > 100U) {
        brightness = 100U;
    }
    if (s_settings.brightness != brightness) {
        s_settings.brightness = brightness;
        mark_dirty();
    }
}

void echoear_settings_set_volume(uint8_t volume)
{
    if (volume > 100U) {
        volume = 100U;
    }
    if (s_settings.volume != volume) {
        s_settings.volume = volume;
        mark_dirty();
    }
}

void echoear_settings_set_animation_speed(float speed)
{
    if (speed < 0.25f) {
        speed = 0.25f;
    } else if (speed > 3.0f) {
        speed = 3.0f;
    }
    if (s_settings.animation_speed != speed) {
        s_settings.animation_speed = speed;
        mark_dirty();
    }
}

void echoear_settings_set_car_mode(bool enabled)
{
    if (s_settings.car_mode != enabled) {
        s_settings.car_mode = enabled;
        mark_dirty();
    }
}

void echoear_settings_set_hands_free(bool enabled)
{
    if (s_settings.hands_free != enabled) {
        s_settings.hands_free = enabled;
        mark_dirty();
    }
}

void echoear_settings_set_show_battery(bool enabled)
{
    if (s_settings.show_battery != enabled) {
        s_settings.show_battery = enabled;
        mark_dirty();
    }
}

void echoear_settings_set_show_subtitles(bool enabled)
{
    if (s_settings.show_subtitles != enabled) {
        s_settings.show_subtitles = enabled;
        mark_dirty();
    }
}

void echoear_settings_set_assistant_voice(echoear_assistant_voice_t voice)
{
    if (voice < ECHOEAR_ASSISTANT_VOICE_LEDA ||
        voice >= ECHOEAR_ASSISTANT_VOICE_COUNT) {
        voice = ECHOEAR_ASSISTANT_VOICE_LEDA;
    }
    if (s_settings.assistant_voice != voice) {
        s_settings.assistant_voice = voice;
        mark_dirty();
    }
}

bool echoear_settings_save(void)
{
    if (!s_settings.open) {
        return false;
    }
    if (s_ops.save != NULL &&
        !s_ops.save(s_settings.brightness, s_settings.volume,
                    s_settings.animation_speed, s_settings.car_mode,
                    s_settings.hands_free, s_settings.show_battery,
                    s_settings.show_subtitles,
                    s_settings.assistant_voice)) {
        s_settings.failures++;
        s_settings.generation++;
        return false;
    }
    echoear_app_state_set_brightness(s_settings.brightness);
    echoear_app_state_set_volume(s_settings.volume);
    echoear_app_state_set_animation_speed(s_settings.animation_speed);
    echoear_app_state_set_car_mode(s_settings.car_mode);
    echoear_app_state_set_hands_free(s_settings.hands_free);
    echoear_app_state_set_show_battery(s_settings.show_battery);
    echoear_app_state_set_show_subtitles(s_settings.show_subtitles);
    echoear_app_state_set_assistant_voice(s_settings.assistant_voice);
    s_settings.dirty = false;
    s_settings.saves++;
    s_settings.generation++;
    return true;
}

void echoear_settings_cancel(void)
{
    if (!s_settings.open) {
        return;
    }
    load_values();
    s_settings.page = ECHOEAR_SETTINGS_PAGE_ROOT;
    s_settings.cancels++;
    s_settings.generation++;
}

bool echoear_settings_request_wifi_setup(void)
{
    if (!s_settings.open || s_ops.request_wifi_setup == NULL ||
        !s_ops.request_wifi_setup()) {
        s_settings.failures++;
        s_settings.generation++;
        return false;
    }
    return true;
}

bool echoear_settings_request_factory_reset(void)
{
    if (!s_settings.open ||
        s_settings.page != ECHOEAR_SETTINGS_PAGE_RESET_CONFIRM ||
        s_ops.request_factory_reset == NULL || !s_ops.request_factory_reset()) {
        s_settings.failures++;
        s_settings.generation++;
        return false;
    }
    return true;
}

const char *echoear_settings_page_name(echoear_settings_page_t page)
{
    switch (page) {
        case ECHOEAR_SETTINGS_PAGE_ROOT: return "root";
        case ECHOEAR_SETTINGS_PAGE_DISPLAY: return "display";
        case ECHOEAR_SETTINGS_PAGE_AUDIO: return "audio";
        case ECHOEAR_SETTINGS_PAGE_CAR_MODE: return "car_mode";
        case ECHOEAR_SETTINGS_PAGE_HANDS_FREE: return "hands_free";
        case ECHOEAR_SETTINGS_PAGE_BATTERY: return "battery";
        case ECHOEAR_SETTINGS_PAGE_SUBTITLES: return "subtitles";
        case ECHOEAR_SETTINGS_PAGE_VOICE: return "voice";
        case ECHOEAR_SETTINGS_PAGE_SYSTEM: return "system";
        case ECHOEAR_SETTINGS_PAGE_RESET_CONFIRM: return "reset_confirm";
        default: return "unknown";
    }
}


const char *echoear_assistant_voice_name(echoear_assistant_voice_t voice)
{
    switch (voice) {
        case ECHOEAR_ASSISTANT_VOICE_LEDA: return "Leda";
        case ECHOEAR_ASSISTANT_VOICE_SULAFAT: return "Sulafat";
        case ECHOEAR_ASSISTANT_VOICE_AOEDE: return "Aoede";
        case ECHOEAR_ASSISTANT_VOICE_ACHIRD: return "Achird";
        case ECHOEAR_ASSISTANT_VOICE_KORE: return "Kore";
        default: return "Leda";
    }
}

const char *echoear_assistant_voice_label(echoear_assistant_voice_t voice)
{
    switch (voice) {
        case ECHOEAR_ASSISTANT_VOICE_LEDA: return "Cute & bright";
        case ECHOEAR_ASSISTANT_VOICE_SULAFAT: return "Warm";
        case ECHOEAR_ASSISTANT_VOICE_AOEDE: return "Soft & breezy";
        case ECHOEAR_ASSISTANT_VOICE_ACHIRD: return "Friendly";
        case ECHOEAR_ASSISTANT_VOICE_KORE: return "Firm";
        default: return "Cute & bright";
    }
}
