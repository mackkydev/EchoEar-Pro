#ifndef ECHOEAR_PROVISIONING_H
#define ECHOEAR_PROVISIONING_H

#include <stdbool.h>
#include <stdint.h>

#define ECHOEAR_PROVISIONING_SSID_MAX 33
#define ECHOEAR_PROVISIONING_IP_MAX   16

typedef enum
{
    ECHOEAR_PROVISIONING_NOT_STARTED = 0,
    ECHOEAR_PROVISIONING_CHECKING,
    ECHOEAR_PROVISIONING_AP_STARTING,
    ECHOEAR_PROVISIONING_AP_READY,
    ECHOEAR_PROVISIONING_CLIENT_CONNECTED,
    ECHOEAR_PROVISIONING_SCANNING,
    ECHOEAR_PROVISIONING_WIFI_CONNECTING,
    ECHOEAR_PROVISIONING_WIFI_CONNECTED,
    ECHOEAR_PROVISIONING_SAVING,
    ECHOEAR_PROVISIONING_COMPLETED,
    ECHOEAR_PROVISIONING_ERROR
} echoear_provisioning_state_t;

typedef enum
{
    ECHOEAR_PROVISIONING_ERROR_NONE = 0,
    ECHOEAR_PROVISIONING_ERROR_AP_START_FAILED,
    ECHOEAR_PROVISIONING_ERROR_SCAN_FAILED,
    ECHOEAR_PROVISIONING_ERROR_NETWORK_NOT_FOUND,
    ECHOEAR_PROVISIONING_ERROR_AUTH_FAILED,
    ECHOEAR_PROVISIONING_ERROR_DHCP_FAILED,
    ECHOEAR_PROVISIONING_ERROR_SAVE_FAILED,
    ECHOEAR_PROVISIONING_ERROR_TIMEOUT,
    ECHOEAR_PROVISIONING_ERROR_UNKNOWN
} echoear_provisioning_error_t;

typedef struct
{
    echoear_provisioning_state_t state;
    echoear_provisioning_error_t error;

    bool enabled;
    bool setup_completed;
    bool client_connected;

    char ap_ssid[ECHOEAR_PROVISIONING_SSID_MAX];
    char target_ssid[ECHOEAR_PROVISIONING_SSID_MAX];
    char ip_address[ECHOEAR_PROVISIONING_IP_MAX];

    int16_t wifi_rssi;
} echoear_provisioning_t;

void echoear_provisioning_init(void);
void echoear_provisioning_reset(void);

echoear_provisioning_t *echoear_provisioning_get(void);

void echoear_provisioning_set_state(
    echoear_provisioning_state_t state);

void echoear_provisioning_set_error(
    echoear_provisioning_error_t error);

void echoear_provisioning_set_enabled(
    bool enabled);

void echoear_provisioning_set_setup_completed(
    bool completed);

void echoear_provisioning_set_client_connected(
    bool connected);

void echoear_provisioning_set_ap_ssid(
    const char *ssid);

void echoear_provisioning_set_target_ssid(
    const char *ssid);

void echoear_provisioning_set_ip_address(
    const char *ip_address);

void echoear_provisioning_set_wifi_rssi(
    int16_t rssi);

bool echoear_provisioning_parse_state(
    const char *value,
    echoear_provisioning_state_t *state);

bool echoear_provisioning_parse_error(
    const char *value,
    echoear_provisioning_error_t *error);

const char *echoear_provisioning_state_name(
    echoear_provisioning_state_t state);

const char *echoear_provisioning_error_name(
    echoear_provisioning_error_t error);

bool echoear_provisioning_is_active(void);
bool echoear_provisioning_is_terminal(void);

#endif
