#include "echoear_pro_ui.h"
#include "echoear_app_state.h"
#include "echoear_provisioning.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COLOR_BG 0x000000
#define COLOR_CYAN 0x43F5E8
#define COLOR_TEXT 0xEAFBFF

typedef struct
{
    const char **frames;
    uint8_t frame_count;
    const uint16_t *frame_durations_ms;
    uint16_t fallback_interval_ms;
    bool show_status_bar;
    int16_t face_offset_y;
} echoear_anim_t;

#define ARRAY_COUNT(items) ((uint8_t)(sizeof(items) / sizeof((items)[0])))

static lv_obj_t *screen_circle;
static lv_obj_t *face_img;
static lv_obj_t *status_bar;
static lv_obj_t *status_line_main;
static lv_obj_t *status_line_info;

static lv_timer_t *anim_timer;
static const echoear_anim_t *current_anim;
static uint8_t current_frame;
static echoear_face_state_t current_state;
static bool current_state_valid;

/* ---------- frame paths ------------------------------------------------------------ */
/* ถ้ารูปไม่ขึ้นทีหลัง เดี๋ยวค่อยเปลี่ยน path เป็น A:/assets/... */

/* ---------- Normal ------------------------------------------------------------ */
static const char *normal_angry[] = {
    "A:assets/faces/normal/angry/face_angry_01.png",
    "A:assets/faces/normal/angry/face_angry_02.png",
    "A:assets/faces/normal/angry/face_angry_03.png"};

static const char *normal_confused[] = {
    "A:assets/faces/normal/confused/face_confused_01.png",
    "A:assets/faces/normal/confused/face_confused_02.png",
    "A:assets/faces/normal/confused/face_confused_03.png"};

static const char *normal_happy[] = {
    "A:assets/faces/normal/happy/face_happy_01.png",
    "A:assets/faces/normal/happy/face_happy_02.png",
    "A:assets/faces/normal/happy/face_happy_03.png"};

static const char *normal_idle[] = {
    "A:assets/faces/normal/idle/face_idle_01.png",
    "A:assets/faces/normal/idle/face_idle_02.png",
    "A:assets/faces/normal/idle/face_idle_03.png"};

static const char *normal_listening[] = {
    "A:assets/faces/normal/listening/face_listening_01.png",
    "A:assets/faces/normal/listening/face_listening_02.png",
    "A:assets/faces/normal/listening/face_listening_03.png"};

static const char *normal_sad[] = {
    "A:assets/faces/normal/sad/face_sad_01.png",
    "A:assets/faces/normal/sad/face_sad_02.png"};

static const char *normal_sleeping[] = {
    "A:assets/faces/normal/sleeping/face_sleeping_01.png",
    "A:assets/faces/normal/sleeping/face_sleeping_02.png"};

static const char *normal_speaking[] = {
    "A:assets/faces/normal/speaking/face_speaking_01.png",
    "A:assets/faces/normal/speaking/face_speaking_02.png",
    "A:assets/faces/normal/speaking/face_speaking_03.png",
    "A:assets/faces/normal/speaking/face_speaking_04.png"};

static const char *normal_surprised[] = {
    "A:assets/faces/normal/surprised/face_surprised_01.png",
    "A:assets/faces/normal/surprised/face_surprised_02.png",
    "A:assets/faces/normal/surprised/face_surprised_03.png"};

static const char *normal_thinking[] = {
    "A:assets/faces/normal/thinking/face_thinking_01.png",
    "A:assets/faces/normal/thinking/face_thinking_02.png",
    "A:assets/faces/normal/thinking/face_thinking_03.png"};

static const char *normal_wink[] = {
    "A:assets/faces/normal/wink/face_wink_01.png",
    "A:assets/faces/normal/wink/face_wink_02.png"};

/* ---------- System ------------------------------------------------------------ */
static const char *system_error[] = {
    "A:assets/faces/system/error/face_error_01.png",
    "A:assets/faces/system/error/face_error_02.png"};

static const char *system_low_battery[] = {
    "A:assets/faces/system/low_battery/face_low_battery_01.png",
    "A:assets/faces/system/low_battery/face_low_battery_02.png"};

static const char *system_ota_updating[] = {
    "A:assets/faces/system/ota_updating/face_ota_updating_01.png",
    "A:assets/faces/system/ota_updating/face_ota_updating_02.png",
    "A:assets/faces/system/ota_updating/face_ota_updating_03.png",
    "A:assets/faces/system/ota_updating/face_ota_updating_04.png",
    "A:assets/faces/system/ota_updating/face_ota_updating_05.png",
    "A:assets/faces/system/ota_updating/face_ota_updating_06.png",
    "A:assets/faces/system/ota_updating/face_ota_updating_07.png",
    "A:assets/faces/system/ota_updating/face_ota_updating_08.png"};

static const char *system_wifi_setup[] = {
    "A:assets/faces/system/wifi_setup/face_wifi_setup_01.png",
    "A:assets/faces/system/wifi_setup/face_wifi_setup_02.png"};

/* ---------- Legacy car / OBD ------------------------------------------------------------ */
static const char *car_obd_connecting[] = {
    "A:assets/faces/car/obd_connecting/face_obd_connecting_01.png",
    "A:assets/faces/car/obd_connecting/face_obd_connecting_02.png",
    "A:assets/faces/car/obd_connecting/face_obd_connecting_03.png"};

static const char *car_obd_error[] = {
    "A:assets/faces/car/obd_error/face_obd_error_01.png",
    "A:assets/faces/car/obd_error/face_obd_error_02.png"};

static const char *car_obd_ready[] = {
    "A:assets/faces/car/obd_ready/face_obd_ready_01.png",
    "A:assets/faces/car/obd_ready/face_obd_ready_02.png",
    "A:assets/faces/car/obd_ready/face_obd_ready_03.png"};

/* ---------- Module 3B dedicated car faces -------------------------------------------- */
static const char *car_charging[] = {
    "A:assets/faces/car/charging/face_charging_01.png",
    "A:assets/faces/car/charging/face_charging_02.png",
    "A:assets/faces/car/charging/face_charging_03.png",
    "A:assets/faces/car/charging/face_charging_04.png"};

static const char *car_low_battery[] = {
    "A:assets/faces/car/low_battery/face_low_battery_01.png",
    "A:assets/faces/car/low_battery/face_low_battery_02.png"};

static const char *car_door_open[] = {
    "A:assets/faces/car/door_open/face_door_open_01.png",
    "A:assets/faces/car/door_open/face_door_open_02.png",
    "A:assets/faces/car/door_open/face_door_open_03.png"};

static const char *car_cloud_stale[] = {
    "A:assets/faces/car/cloud_stale/face_cloud_stale_01.png",
    "A:assets/faces/car/cloud_stale/face_cloud_stale_02.png"};

static const char *car_cloud_offline[] = {
    "A:assets/faces/car/cloud_offline/face_cloud_offline_01.png",
    "A:assets/faces/car/cloud_offline/face_cloud_offline_02.png",
    "A:assets/faces/car/cloud_offline/face_cloud_offline_03.png"};

static const char *car_driving[] = {
    "A:assets/faces/car/driving/face_driving_01.png",
    "A:assets/faces/car/driving/face_driving_02.png",
    "A:assets/faces/car/driving/face_driving_03.png"};

/* ---------- animation timing ------------------------------------------------------------ */
static const uint16_t timing_normal_idle[] = {
    1800, 140, 1800};

static const uint16_t timing_normal_listening[] = {
    650, 250, 650};

static const uint16_t timing_normal_thinking[] = {
    700, 450, 700};

static const uint16_t timing_normal_speaking[] = {
    220, 300, 220, 300};

static const uint16_t timing_normal_happy[] = {
    650, 300, 650};

static const uint16_t timing_normal_confused[] = {
    250, 350, 250};

static const uint16_t timing_normal_sad[] = {
    1000, 300};

static const uint16_t timing_normal_sleeping[] = {
    1300, 1300};

static const uint16_t timing_normal_wink[] = {
    900, 220};

static const uint16_t timing_normal_angry[] = {
    650, 350, 650};

static const uint16_t timing_normal_surprised[] = {
    550, 300, 550};

static const uint16_t timing_car_charging[] = {
    550, 350, 550, 350};

static const uint16_t timing_car_low_battery[] = {
    900, 500};

static const uint16_t timing_car_door_open[] = {
    700, 350, 700};

static const uint16_t timing_car_cloud_stale[] = {
    1000, 1000};

static const uint16_t timing_car_cloud_offline[] = {
    700, 450, 700};

static const uint16_t timing_car_driving[] = {
    550, 350, 550};

/* ---------- animations ------------------------------------------------------------ */
static const echoear_anim_t anim_normal_angry = {
    normal_angry, ARRAY_COUNT(normal_angry), timing_normal_angry, 260, false, 0};
static const echoear_anim_t anim_normal_confused = {
    normal_confused, ARRAY_COUNT(normal_confused), timing_normal_confused, 360, false, 0};
static const echoear_anim_t anim_normal_happy = {
    normal_happy, ARRAY_COUNT(normal_happy), timing_normal_happy, 340, false, 0};
static const echoear_anim_t anim_normal_idle = {
    normal_idle, ARRAY_COUNT(normal_idle), timing_normal_idle, 600, false, 0};
static const echoear_anim_t anim_normal_listening = {
    normal_listening, ARRAY_COUNT(normal_listening), timing_normal_listening, 360, false, 0};
static const echoear_anim_t anim_normal_sad = {
    normal_sad, ARRAY_COUNT(normal_sad), timing_normal_sad, 560, false, 0};
static const echoear_anim_t anim_normal_sleeping = {
    normal_sleeping, ARRAY_COUNT(normal_sleeping), timing_normal_sleeping, 900, false, 0};
static const echoear_anim_t anim_normal_speaking = {
    normal_speaking, ARRAY_COUNT(normal_speaking), timing_normal_speaking, 160, false, 0};
static const echoear_anim_t anim_normal_surprised = {
    normal_surprised, ARRAY_COUNT(normal_surprised), timing_normal_surprised, 220, false, 0};
static const echoear_anim_t anim_normal_thinking = {
    normal_thinking, ARRAY_COUNT(normal_thinking), timing_normal_thinking, 450, false, 0};
static const echoear_anim_t anim_normal_wink = {
    normal_wink, ARRAY_COUNT(normal_wink), timing_normal_wink, 300, false, 0};

static const echoear_anim_t anim_system_error = {
    system_error, ARRAY_COUNT(system_error), NULL, 450, false, 0};
static const echoear_anim_t anim_system_low_battery = {
    system_low_battery, ARRAY_COUNT(system_low_battery), NULL, 550, false, 0};
static const echoear_anim_t anim_system_ota_updating = {
    system_ota_updating, ARRAY_COUNT(system_ota_updating), NULL, 160, false, 0};
static const echoear_anim_t anim_system_wifi_setup = {
    system_wifi_setup, ARRAY_COUNT(system_wifi_setup), NULL, 500, false, 0};

/* Legacy assets are centered higher because they do not reserve the bottom text area. */
static const echoear_anim_t anim_car_obd_connecting = {
    car_obd_connecting, ARRAY_COUNT(car_obd_connecting), NULL, 320, true, -48};
static const echoear_anim_t anim_car_obd_error = {
    car_obd_error, ARRAY_COUNT(car_obd_error), NULL, 500, true, -48};
static const echoear_anim_t anim_car_obd_ready = {
    car_obd_ready, ARRAY_COUNT(car_obd_ready), NULL, 350, true, -48};
static const echoear_anim_t anim_car_parked = {
    car_obd_ready, ARRAY_COUNT(car_obd_ready), NULL, 420, true, -48};

/* New Module 3B assets are full 360x360 templates with icon at the top and
   an intentionally empty status area at the bottom, so offset Y remains zero. */
static const echoear_anim_t anim_car_charging = {
    car_charging, ARRAY_COUNT(car_charging), timing_car_charging, 240, true, 0};
static const echoear_anim_t anim_car_low_battery = {
    car_low_battery, ARRAY_COUNT(car_low_battery), timing_car_low_battery, 420, true, 0};
static const echoear_anim_t anim_car_door_open = {
    car_door_open, ARRAY_COUNT(car_door_open), timing_car_door_open, 260, true, 0};
static const echoear_anim_t anim_car_cloud_stale = {
    car_cloud_stale, ARRAY_COUNT(car_cloud_stale), timing_car_cloud_stale, 500, true, 0};
static const echoear_anim_t anim_car_cloud_offline = {
    car_cloud_offline, ARRAY_COUNT(car_cloud_offline), timing_car_cloud_offline, 320, true, 0};
static const echoear_anim_t anim_car_driving = {
    car_driving, ARRAY_COUNT(car_driving), timing_car_driving, 220, true, 0};

static uint32_t animation_period_ms(const echoear_anim_t *anim, uint8_t frame_index)
{
    echoear_app_state_t *state_data = echoear_app_state_get();
    uint16_t raw_period = anim->fallback_interval_ms;
    float speed = state_data->animation_speed;
    uint32_t period;

    if (anim->frame_durations_ms != NULL && frame_index < anim->frame_count)
    {
        raw_period = anim->frame_durations_ms[frame_index];
    }
    if (speed < 0.25f)
    {
        speed = 0.25f;
    }

    period = (uint32_t)((float)raw_period / speed);
    return period < 60U ? 60U : period;
}

static void anim_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    if (current_anim == NULL || current_anim->frame_count == 0)
    {
        return;
    }

    current_frame++;
    if (current_frame >= current_anim->frame_count)
    {
        current_frame = 0;
    }

    lv_image_set_src(face_img, current_anim->frames[current_frame]);
    lv_timer_set_period(anim_timer, animation_period_ms(current_anim, current_frame));
}

static const char *speed_source_label(echoear_speed_source_t source)
{
    switch (source)
    {
    case ECHOEAR_SPEED_SOURCE_PHONE_GPS:
        return "GPS";
    case ECHOEAR_SPEED_SOURCE_OBD:
        return "OBD";
    case ECHOEAR_SPEED_SOURCE_VEHICLE:
        return "VEHICLE";
    case ECHOEAR_SPEED_SOURCE_UNAVAILABLE:
    default:
        return "--";
    }
}

static void set_label_text_if_changed(lv_obj_t *label, const char *text)
{
    const char *current_text;

    if (label == NULL || text == NULL)
    {
        return;
    }

    current_text = lv_label_get_text(label);
    if (current_text == NULL || strcmp(current_text, text) != 0)
    {
        lv_label_set_text(label, text);
    }
}

static lv_obj_t *make_status_bar(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 284, 56);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static void update_status_lines_from_state(void)
{
    echoear_app_state_t *state = echoear_app_state_get();
    echoear_vehicle_state_t *vehicle = &state->vehicle;
    char main_line[72];
    char info_line[160];

    if (current_state == ECHOEAR_FACE_CAR_OBD_CONNECTING)
    {
        snprintf(main_line, sizeof(main_line), "VEHICLE CONNECTING");
        snprintf(info_line, sizeof(info_line), "WAITING FOR VEHICLE DATA...");
    }
    else if (current_state == ECHOEAR_FACE_CAR_OBD_ERROR)
    {
        snprintf(main_line, sizeof(main_line), "VEHICLE ERROR");
        snprintf(info_line, sizeof(info_line), "CHECK GATEWAY OR DATA CONNECTION");
    }
    else if (current_state == ECHOEAR_FACE_CAR_OBD_READY)
    {
        snprintf(main_line, sizeof(main_line), "%s | %s",
                 vehicle->status[0] != '\0' ? vehicle->status : "VEHICLE READY",
                 vehicle->locked ? "LOCKED" : "UNLOCKED");
        snprintf(info_line, sizeof(info_line),
                 "SOC %d%% | RANGE %d km | SPEED %.0f km/h",
                 vehicle->soc_percent,
                 vehicle->range_km,
                 vehicle->speed_kph);
    }
    else
    {
        switch (vehicle->scenario)
        {
        case ECHOEAR_VEHICLE_SCENARIO_CHARGING:
            snprintf(main_line, sizeof(main_line),
                     "CHARGING | SOC %d%% | %.1f kW",
                     vehicle->soc_percent,
                     vehicle->charge_power_kw);
            snprintf(info_line, sizeof(info_line),
                     "LIMIT %d%% | RANGE %d km | %s",
                     vehicle->charge_limit_percent,
                     vehicle->range_km,
                     vehicle->plugged_in ? "PLUGGED IN" : "NOT PLUGGED IN");
            break;

        case ECHOEAR_VEHICLE_SCENARIO_LOW_BATTERY:
            snprintf(main_line, sizeof(main_line),
                     "LOW BATTERY | SOC %d%%",
                     vehicle->soc_percent);
            snprintf(info_line, sizeof(info_line),
                     "CHARGE SOON | EST. RANGE %d km",
                     vehicle->range_km);
            break;

        case ECHOEAR_VEHICLE_SCENARIO_DOOR_OPEN:
            snprintf(main_line, sizeof(main_line),
                     "DOOR OPEN | %s",
                     vehicle->locked ? "LOCKED" : "UNLOCKED");
            snprintf(info_line, sizeof(info_line),
                     "SOC %d%% | RANGE %d km",
                     vehicle->soc_percent,
                     vehicle->range_km);
            break;

        case ECHOEAR_VEHICLE_SCENARIO_CLOUD_STALE:
            snprintf(main_line, sizeof(main_line), "DATA STALE");
            snprintf(info_line, sizeof(info_line),
                     "LAST KNOWN DATA | SOC %d%% | RANGE %d km | REFRESH REQUIRED",
                     vehicle->soc_percent,
                     vehicle->range_km);
            break;

        case ECHOEAR_VEHICLE_SCENARIO_CLOUD_OFFLINE:
            snprintf(main_line, sizeof(main_line), "CLOUD OFFLINE");
            snprintf(info_line, sizeof(info_line),
                     "RETRYING CONNECTION TO GATEWAY...");
            break;

        case ECHOEAR_VEHICLE_SCENARIO_DRIVING_GPS:
            snprintf(main_line, sizeof(main_line),
                     "DRIVING | %.0f km/h",
                     vehicle->speed_kph);
            snprintf(info_line, sizeof(info_line),
                     "RANGE %d km | SOURCE %s | BATTERY %d%%",
                     vehicle->range_km,
                     speed_source_label(vehicle->speed_source),
                     vehicle->soc_percent);
            break;

        case ECHOEAR_VEHICLE_SCENARIO_PARKED:
        default:
            snprintf(main_line, sizeof(main_line), "%s | %s",
                     vehicle->status[0] != '\0' ? vehicle->status : "VEHICLE READY",
                     vehicle->locked ? "LOCKED" : "UNLOCKED");
            snprintf(info_line, sizeof(info_line),
                     "SOC %d%% | RANGE %d km",
                     vehicle->soc_percent,
                     vehicle->range_km);
            break;
        }
    }

    set_label_text_if_changed(status_line_main, main_line);
    set_label_text_if_changed(status_line_info, info_line);
}

void echoear_pro_ui_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x303030), 0); /*เปลี่ยนสีพื้นหลังนอกจอ (scr, lv_color_hex(COLOR_BG), 0)*/
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* วงกลมจำลองจอจริง */
    screen_circle = lv_obj_create(scr);
    lv_obj_remove_style_all(screen_circle);
    lv_obj_set_size(screen_circle, 360, 360);
    lv_obj_center(screen_circle);
    lv_obj_set_style_radius(screen_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(screen_circle, lv_color_hex(COLOR_BG), 0); /*เปลี่ยนสีพื้นจอ (scr, lv_color_hex((0x000000)), 0)*/
    lv_obj_set_style_bg_opa(screen_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_clip_corner(screen_circle, true, 0);

    face_img = lv_image_create(screen_circle);
    lv_obj_align(face_img, LV_ALIGN_CENTER, 0, 0);

    status_bar = make_status_bar(screen_circle);

    status_line_main = lv_label_create(status_bar);
    lv_obj_set_size(status_line_main, 264, 22);
    lv_obj_align(status_line_main, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(status_line_main, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_set_style_text_align(status_line_main, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(status_line_main, LV_LABEL_LONG_CLIP);

    status_line_info = lv_label_create(status_bar);
    lv_obj_set_size(status_line_info, 218, 20);
    lv_obj_align(status_line_info, LV_ALIGN_TOP_MID, 0, 27);
    lv_obj_set_style_text_color(status_line_info, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_align(status_line_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_anim_duration(status_line_info, 6500, 0);
    lv_label_set_long_mode(status_line_info, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_add_flag(status_bar, LV_OBJ_FLAG_HIDDEN);

    anim_timer = lv_timer_create(anim_timer_cb, 400, NULL);
}

void echoear_pro_ui_set_state(echoear_face_state_t state)
{
    if (current_state_valid && current_state == state && current_anim != NULL)
    {
        if (current_anim->show_status_bar)
        {
            update_status_lines_from_state();
        }
        return;
    }

    switch (state)
    {
    case ECHOEAR_FACE_NORMAL_ANGRY:
        current_anim = &anim_normal_angry;
        break;
    case ECHOEAR_FACE_NORMAL_CONFUSED:
        current_anim = &anim_normal_confused;
        break;
    case ECHOEAR_FACE_NORMAL_HAPPY:
        current_anim = &anim_normal_happy;
        break;
    case ECHOEAR_FACE_NORMAL_SAD:
        current_anim = &anim_normal_sad;
        break;
    case ECHOEAR_FACE_NORMAL_SURPRISED:
        current_anim = &anim_normal_surprised;
        break;
    case ECHOEAR_FACE_NORMAL_WINK:
        current_anim = &anim_normal_wink;
        break;
    case ECHOEAR_FACE_NORMAL_IDLE:
        current_anim = &anim_normal_idle;
        break;
    case ECHOEAR_FACE_NORMAL_LISTENING:
        current_anim = &anim_normal_listening;
        break;
    case ECHOEAR_FACE_NORMAL_THINKING:
        current_anim = &anim_normal_thinking;
        break;
    case ECHOEAR_FACE_NORMAL_SPEAKING:
        current_anim = &anim_normal_speaking;
        break;
    case ECHOEAR_FACE_NORMAL_SLEEPING:
        current_anim = &anim_normal_sleeping;
        break;
    case ECHOEAR_FACE_SYSTEM_ERROR:
        current_anim = &anim_system_error;
        break;
    case ECHOEAR_FACE_SYSTEM_LOW_BATTERY:
        current_anim = &anim_system_low_battery;
        break;
    case ECHOEAR_FACE_SYSTEM_OTA_UPDATING:
        current_anim = &anim_system_ota_updating;
        break;
    case ECHOEAR_FACE_SYSTEM_WIFI_SETUP:
        current_anim = &anim_system_wifi_setup;
        break;
    case ECHOEAR_FACE_CAR_OBD_CONNECTING:
        current_anim = &anim_car_obd_connecting;
        break;
    case ECHOEAR_FACE_CAR_OBD_ERROR:
        current_anim = &anim_car_obd_error;
        break;
    case ECHOEAR_FACE_CAR_OBD_READY:
        current_anim = &anim_car_obd_ready;
        break;
    case ECHOEAR_FACE_CAR_PARKED:
        current_anim = &anim_car_parked;
        break;
    case ECHOEAR_FACE_CAR_CHARGING:
        current_anim = &anim_car_charging;
        break;
    case ECHOEAR_FACE_CAR_LOW_BATTERY:
        current_anim = &anim_car_low_battery;
        break;
    case ECHOEAR_FACE_CAR_DOOR_OPEN:
        current_anim = &anim_car_door_open;
        break;
    case ECHOEAR_FACE_CAR_CLOUD_STALE:
        current_anim = &anim_car_cloud_stale;
        break;
    case ECHOEAR_FACE_CAR_CLOUD_OFFLINE:
        current_anim = &anim_car_cloud_offline;
        break;
    case ECHOEAR_FACE_CAR_DRIVING:
        current_anim = &anim_car_driving;
        break;
    default:
        current_anim = &anim_normal_idle;
        break;
    }

    current_state = state;
    current_state_valid = true;
    current_frame = 0;
    lv_image_set_src(face_img, current_anim->frames[0]);
    lv_timer_set_period(anim_timer, animation_period_ms(current_anim, current_frame));

    lv_obj_align(face_img, LV_ALIGN_CENTER, 0, current_anim->face_offset_y);

    if (current_anim->show_status_bar)
    {
        lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_HIDDEN);
        update_status_lines_from_state();
    }
    else
    {
        lv_obj_add_flag(status_bar, LV_OBJ_FLAG_HIDDEN);
    }
}

void echoear_pro_ui_apply_interaction_state(void)
{
    echoear_app_state_t *state = echoear_app_state_get();

    switch (state->interaction_state)
    {
    case ECHOEAR_INTERACTION_LISTENING:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_LISTENING);
        break;
    case ECHOEAR_INTERACTION_THINKING:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_THINKING);
        break;
    case ECHOEAR_INTERACTION_SPEAKING:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_SPEAKING);
        break;
    case ECHOEAR_INTERACTION_HAPPY:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_HAPPY);
        break;
    case ECHOEAR_INTERACTION_CONFUSED:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_CONFUSED);
        break;
    case ECHOEAR_INTERACTION_SAD:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_SAD);
        break;
    case ECHOEAR_INTERACTION_SLEEPING:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_SLEEPING);
        break;
    case ECHOEAR_INTERACTION_WINK:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_WINK);
        break;
    case ECHOEAR_INTERACTION_ANGRY:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_ANGRY);
        break;
    case ECHOEAR_INTERACTION_SURPRISED:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_SURPRISED);
        break;
    case ECHOEAR_INTERACTION_IDLE:
    default:
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_IDLE);
        break;
    }
}

void echoear_pro_ui_apply_vehicle_state(void)
{
    echoear_app_state_t *state = echoear_app_state_get();

    if (!state->car_mode)
    {
        echoear_pro_ui_apply_interaction_state();
        return;
    }

    switch (state->vehicle.scenario)
    {
    case ECHOEAR_VEHICLE_SCENARIO_CHARGING:
        echoear_pro_ui_set_state(ECHOEAR_FACE_CAR_CHARGING);
        break;
    case ECHOEAR_VEHICLE_SCENARIO_LOW_BATTERY:
        echoear_pro_ui_set_state(ECHOEAR_FACE_CAR_LOW_BATTERY);
        break;
    case ECHOEAR_VEHICLE_SCENARIO_DOOR_OPEN:
        echoear_pro_ui_set_state(ECHOEAR_FACE_CAR_DOOR_OPEN);
        break;
    case ECHOEAR_VEHICLE_SCENARIO_CLOUD_STALE:
        echoear_pro_ui_set_state(ECHOEAR_FACE_CAR_CLOUD_STALE);
        break;
    case ECHOEAR_VEHICLE_SCENARIO_CLOUD_OFFLINE:
        echoear_pro_ui_set_state(ECHOEAR_FACE_CAR_CLOUD_OFFLINE);
        break;
    case ECHOEAR_VEHICLE_SCENARIO_DRIVING_GPS:
        echoear_pro_ui_set_state(ECHOEAR_FACE_CAR_DRIVING);
        break;
    case ECHOEAR_VEHICLE_SCENARIO_PARKED:
    default:
        echoear_pro_ui_set_state(ECHOEAR_FACE_CAR_PARKED);
        break;
    }
}

void echoear_pro_ui_apply_app_state(void)
{
    echoear_app_state_t *state = echoear_app_state_get();

    if (state->car_mode)
    {
        echoear_pro_ui_apply_vehicle_state();
    }
    else
    {
        echoear_pro_ui_apply_interaction_state();
    }
}

void echoear_pro_ui_refresh(void)
{
    if (current_anim == NULL)
    {
        return;
    }

    if (current_anim->show_status_bar)
    {
        update_status_lines_from_state();
    }

    if (anim_timer != NULL)
    {
        lv_timer_set_period(anim_timer, animation_period_ms(current_anim, current_frame));
    }
}

void echoear_pro_ui_apply_provisioning_state(void)
{
    echoear_provisioning_t *provisioning =
        echoear_provisioning_get();

    if (provisioning == NULL)
    {
        echoear_pro_ui_apply_app_state();
        return;
    }

    if (!provisioning->enabled)
    {
        echoear_pro_ui_apply_app_state();
        return;
    }

    if (provisioning->setup_completed ||
        provisioning->state ==
            ECHOEAR_PROVISIONING_COMPLETED)
    {
        echoear_pro_ui_set_state(
            ECHOEAR_FACE_NORMAL_HAPPY);
        return;
    }

    if (provisioning->state ==
            ECHOEAR_PROVISIONING_ERROR ||
        provisioning->error !=
            ECHOEAR_PROVISIONING_ERROR_NONE)
    {
        echoear_pro_ui_set_state(
            ECHOEAR_FACE_SYSTEM_ERROR);
        return;
    }

    echoear_pro_ui_set_state(
        ECHOEAR_FACE_SYSTEM_WIFI_SETUP);
}