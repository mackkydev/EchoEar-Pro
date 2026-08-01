#include "echoear_app_state.h"
#include <string.h>

static echoear_app_state_t app_state;

static void copy_text(char *dst, size_t dst_size, const char *src)
{
    if(dst == NULL || dst_size == 0) return;

    if(src == NULL) {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

void echoear_app_state_init(void)
{
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

    copy_text(app_state.obd.status, sizeof(app_state.obd.status), "OBD READY");
    app_state.obd.soc_percent = 82;
    app_state.obd.range_km = 478;
    app_state.obd.speed_kmh = 0;
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
    if(brightness > 100) brightness = 100;
    app_state.brightness = brightness;
}

void echoear_app_state_set_volume(uint8_t volume)
{
    if(volume > 100) volume = 100;
    app_state.volume = volume;
}

void echoear_app_state_set_animation_speed(float speed)
{
    if(speed < 0.25f) speed = 0.25f;
    if(speed > 3.0f) speed = 3.0f;
    app_state.animation_speed = speed;
}

void echoear_app_state_set_car_mode(bool enabled)
{
    app_state.car_mode = enabled;
}

void echoear_app_state_set_obd(
    const char *status,
    int soc_percent,
    int range_km,
    int speed_kmh
)
{
    copy_text(app_state.obd.status, sizeof(app_state.obd.status), status);

    app_state.obd.soc_percent = soc_percent;
    app_state.obd.range_km = range_km;
    app_state.obd.speed_kmh = speed_kmh;
}