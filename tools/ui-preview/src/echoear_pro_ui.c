#include "echoear_pro_ui.h"
#include "echoear_app_state.h"
#include <stdbool.h>
#include <stdio.h>

#define COLOR_BG 0x000000
#define COLOR_CYAN 0x43F5E8
#define COLOR_TEXT 0xEAFBFF

typedef struct
{
    const char **frames;
    uint8_t frame_count;
    uint16_t interval_ms;
    bool show_car_panel;
    const char *line1;
    const char *line2;
    const char *line3;
    const char *line4;
} echoear_anim_t;

static lv_obj_t *screen_circle;
static lv_obj_t *face_img;
static lv_obj_t *car_panel;
static lv_obj_t *car_line_1;
static lv_obj_t *car_line_2;
static lv_obj_t *car_line_3;
static lv_obj_t *car_line_4;

static lv_timer_t *anim_timer;
static const echoear_anim_t *current_anim;
static uint8_t current_frame;
static echoear_face_state_t current_state;
static bool current_state_valid;

/* ---------- frame paths ------------------------------------------------------------ */
/* ถ้ารูปไม่ขึ้นทีหลัง เดี๋ยวค่อยเปลี่ยน path เป็น A:/assets/... */

/* ---------- Nomail ---------- */
static const char *normal_angry[] = {
    "A:assets/faces/normal/angry/face_angry_01.png",
    "A:assets/faces/normal/angry/face_angry_02.png",
    "A:assets/faces/normal/angry/face_angry_03.png"};

static const char *normal_confused[] = {
    "A:assets/faces/normal/confused/face_confused_01.png",
    "A:assets/faces/normal/confused/face_confused_02.png",
    "A:assets/faces/normal/confused/face_confused_03.png",
    "A:assets/faces/normal/confused/face_confused_04.png"};

static const char *normal_happy[] = {
    "A:assets/faces/normal/happy/face_happy_01.png",
    "A:assets/faces/normal/happy/face_happy_02.png"};

static const char *normal_idle[] = {
    "A:assets/faces/normal/idle/face_idle_01.png",
    "A:assets/faces/normal/idle/face_idle_02.png",
    "A:assets/faces/normal/idle/face_idle_03.png",
    "A:assets/faces/normal/idle/face_idle_04.png",
    "A:assets/faces/normal/idle/face_idle_05.png"};

static const char *normal_listening[] = {
    "A:assets/faces/normal/listening/face_listening_01.png",
    "A:assets/faces/normal/listening/face_listening_02.png"};

static const char *normal_sad[] = {
    "A:assets/faces/normal/sad/face_sad_01.png",
    "A:assets/faces/normal/sad/face_sad_02.png"};

static const char *normal_sleeping[] = {
    "A:assets/faces/normal/sleeping/face_sleeping_01.png",
    "A:assets/faces/normal/sleeping/face_sleeping_02.png",
    "A:assets/faces/normal/sleeping/face_sleeping_03.png"};

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
    "A:assets/faces/normal/wink/face_wink_02.png",
    "A:assets/faces/normal/wink/face_wink_03.png"};

/* ---------- System ---------- */
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

/* ---------- Car_OBD ---------- */
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

/* ---------- animations ------------------------------------------------------------ */

static const echoear_anim_t anim_normal_angry = {
    normal_angry, 3, 260, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_confused = {
    normal_confused, 4, 320, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_happy = {
    normal_happy, 2, 420, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_idle = {
    normal_idle, 5, 600, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_listening = {
    normal_listening, 2, 380, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_sad = {
    normal_sad, 2, 600, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_sleeping = {
    normal_sleeping, 3, 700, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_speaking = {
    normal_speaking, 4, 160, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_surprised = {
    normal_surprised, 3, 260, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_thinking = {
    normal_thinking, 3, 450, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_normal_wink = {
    normal_wink, 3, 300, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_system_error = {
    system_error, 2, 450, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_system_low_battery = {
    system_low_battery, 2, 550, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_system_ota_updating = {
    system_ota_updating, 8, 160, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_system_wifi_setup = {
    system_wifi_setup, 2, 500, false, NULL, NULL, NULL, NULL};

static const echoear_anim_t anim_car_obd_connecting = {
    car_obd_connecting, 3, 320, true,
    "OBD CONNECTING",
    "SOC --%",
    "RANGE -- km",
    "SPEED -- km/h"};

static const echoear_anim_t anim_car_obd_error = {
    car_obd_error, 3, 500, true,
    "OBD ERROR",
    "SOC --%",
    "RANGE -- km",
    "CHECK CONNECTION"};

static const echoear_anim_t anim_car_obd_ready = {
    car_obd_ready, 3, 350, true,
    "OBD READY",
    "SOC 82%",
    "RANGE 478 km",
    "SPEED 0 km/h"};

/* Module 3A reuses existing graphics. Module 3B can replace each frame set
   with dedicated vehicle-state artwork without changing the state model. */
static const echoear_anim_t anim_car_parked = {
    car_obd_ready, 3, 420, true,
    "VEHICLE READY", "SOC --%", "RANGE -- km", "LOCKED"};

static const echoear_anim_t anim_car_charging = {
    normal_happy, 2, 420, true,
    "CHARGING", "SOC --%", "POWER -- kW", "LIMIT --%"};

static const echoear_anim_t anim_car_low_battery = {
    system_low_battery, 2, 550, true,
    "LOW BATTERY", "SOC --%", "RANGE -- km", "CHARGE SOON"};

static const echoear_anim_t anim_car_door_open = {
    normal_confused, 4, 380, true,
    "DOOR OPEN", "SOC --%", "RANGE -- km", "UNLOCKED"};

static const echoear_anim_t anim_car_cloud_stale = {
    normal_thinking, 3, 650, true,
    "DATA STALE", "SOC --%", "RANGE -- km", "CHECK REFRESH"};

static const echoear_anim_t anim_car_cloud_offline = {
    car_obd_error, 2, 650, true,
    "CLOUD OFFLINE", "SOC --%", "RANGE -- km", "RETRYING..."};

static const echoear_anim_t anim_car_driving = {
    car_obd_ready, 3, 240, true,
    "DRIVING", "SPEED -- km/h", "RANGE -- km", "SOURCE --"};

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
}

static lv_obj_t *make_panel(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 248, 76);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_radius(obj, 18, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x041018), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_70, 0);
    lv_obj_set_style_shadow_width(obj, 24, 0);
    lv_obj_set_style_shadow_spread(obj, 2, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_30, 0);
    return obj;
}

static void update_car_panel_from_state(void)
{
    echoear_app_state_t *state = echoear_app_state_get();
    echoear_vehicle_state_t *vehicle = &state->vehicle;

    char line1[32];
    char line2[32];
    char line3[32];
    char line4[32];

    switch (vehicle->scenario)
    {
    case ECHOEAR_VEHICLE_SCENARIO_CHARGING:
        snprintf(line1, sizeof(line1), "CHARGING");
        snprintf(line2, sizeof(line2), "SOC %d%%", vehicle->soc_percent);
        snprintf(line3, sizeof(line3), "POWER %.1f kW", vehicle->charge_power_kw);
        snprintf(line4, sizeof(line4), "LIMIT %d%%", vehicle->charge_limit_percent);
        break;

    case ECHOEAR_VEHICLE_SCENARIO_LOW_BATTERY:
        snprintf(line1, sizeof(line1), "LOW BATTERY");
        snprintf(line2, sizeof(line2), "SOC %d%%", vehicle->soc_percent);
        snprintf(line3, sizeof(line3), "RANGE %d km", vehicle->range_km);
        snprintf(line4, sizeof(line4), "CHARGE SOON");
        break;

    case ECHOEAR_VEHICLE_SCENARIO_DOOR_OPEN:
        snprintf(line1, sizeof(line1), "DOOR OPEN");
        snprintf(line2, sizeof(line2), "SOC %d%%", vehicle->soc_percent);
        snprintf(line3, sizeof(line3), "RANGE %d km", vehicle->range_km);
        snprintf(line4, sizeof(line4), "%s", vehicle->locked ? "LOCKED" : "UNLOCKED");
        break;

    case ECHOEAR_VEHICLE_SCENARIO_CLOUD_STALE:
        snprintf(line1, sizeof(line1), "DATA STALE");
        snprintf(line2, sizeof(line2), "SOC %d%%", vehicle->soc_percent);
        snprintf(line3, sizeof(line3), "RANGE %d km", vehicle->range_km);
        snprintf(line4, sizeof(line4), "CHECK REFRESH");
        break;

    case ECHOEAR_VEHICLE_SCENARIO_CLOUD_OFFLINE:
        snprintf(line1, sizeof(line1), "CLOUD OFFLINE");
        snprintf(line2, sizeof(line2), "SOC --%%");
        snprintf(line3, sizeof(line3), "RANGE -- km");
        snprintf(line4, sizeof(line4), "RETRYING...");
        break;

    case ECHOEAR_VEHICLE_SCENARIO_DRIVING_GPS:
        snprintf(line1, sizeof(line1), "DRIVING");
        snprintf(line2, sizeof(line2), "SPEED %.0f km/h", vehicle->speed_kph);
        snprintf(line3, sizeof(line3), "RANGE %d km", vehicle->range_km);
        if (vehicle->speed_source == ECHOEAR_SPEED_SOURCE_PHONE_GPS)
            snprintf(line4, sizeof(line4), "SOURCE GPS");
        else if (vehicle->speed_source == ECHOEAR_SPEED_SOURCE_OBD)
            snprintf(line4, sizeof(line4), "SOURCE OBD");
        else if (vehicle->speed_source == ECHOEAR_SPEED_SOURCE_VEHICLE)
            snprintf(line4, sizeof(line4), "SOURCE CAR");
        else
            snprintf(line4, sizeof(line4), "SOURCE --");
        break;

    case ECHOEAR_VEHICLE_SCENARIO_PARKED:
    default:
        snprintf(line1, sizeof(line1), "%s",
                 vehicle->status[0] != '\0' ? vehicle->status : "VEHICLE READY");
        snprintf(line2, sizeof(line2), "SOC %d%%", vehicle->soc_percent);
        snprintf(line3, sizeof(line3), "RANGE %d km", vehicle->range_km);
        snprintf(line4, sizeof(line4), "%s", vehicle->locked ? "LOCKED" : "UNLOCKED");
        break;
    }

    lv_label_set_text(car_line_1, line1);
    lv_label_set_text(car_line_2, line2);
    lv_label_set_text(car_line_3, line3);
    lv_label_set_text(car_line_4, line4);
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
    lv_obj_align(face_img, LV_ALIGN_CENTER, 0, -34);

    car_panel = make_panel(screen_circle);

    car_line_1 = lv_label_create(car_panel);
    lv_obj_set_style_text_color(car_line_1, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_align(car_line_1, LV_ALIGN_TOP_MID, 0, 8);

    car_line_2 = lv_label_create(car_panel);
    lv_obj_set_style_text_color(car_line_2, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(car_line_2, LV_ALIGN_TOP_LEFT, 18, 31);

    car_line_3 = lv_label_create(car_panel);
    lv_obj_set_style_text_color(car_line_3, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(car_line_3, LV_ALIGN_TOP_RIGHT, -18, 31);

    car_line_4 = lv_label_create(car_panel);
    lv_obj_set_style_text_color(car_line_4, lv_color_hex(0x9CFDF5), 0);
    lv_obj_align(car_line_4, LV_ALIGN_TOP_MID, 0, 54);

    lv_obj_add_flag(car_panel, LV_OBJ_FLAG_HIDDEN);

    anim_timer = lv_timer_create(anim_timer_cb, 400, NULL);
}

void echoear_pro_ui_set_state(echoear_face_state_t state)
{
    if (current_state_valid && current_state == state && current_anim != NULL)
    {
        if (current_anim->show_car_panel)
        {
            update_car_panel_from_state();
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
    echoear_app_state_t *state_data = echoear_app_state_get();

    uint32_t period = (uint32_t)((float)current_anim->interval_ms / state_data->animation_speed);
    if (period < 60)
        period = 60;

    lv_timer_set_period(anim_timer, period);

    if (current_anim->show_car_panel)
    {
        lv_obj_remove_flag(car_panel, LV_OBJ_FLAG_HIDDEN);
        update_car_panel_from_state();
    }
    else
    {
        lv_obj_add_flag(car_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void echoear_pro_ui_apply_vehicle_state(void)
{
    echoear_app_state_t *state = echoear_app_state_get();

    if (!state->car_mode)
    {
        echoear_pro_ui_set_state(ECHOEAR_FACE_NORMAL_IDLE);
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

void echoear_pro_ui_refresh(void)
{
    if (current_anim == NULL)
    {
        return;
    }

    if (current_anim->show_car_panel)
    {
        update_car_panel_from_state();
    }

    if (anim_timer != NULL)
    {
        echoear_app_state_t *state_data = echoear_app_state_get();

        uint32_t period = (uint32_t)((float)current_anim->interval_ms / state_data->animation_speed);
        if (period < 60)
            period = 60;

        lv_timer_set_period(anim_timer, period);
    }
}