#ifndef ECHOEAR_WIFI_MANAGER_H
#define ECHOEAR_WIFI_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ECHOEAR_WIFI_MAX_NETWORKS      16
#define ECHOEAR_WIFI_SSID_MAX          33
#define ECHOEAR_WIFI_PASSWORD_MAX      65
#define ECHOEAR_WIFI_BSSID_MAX         18
#define ECHOEAR_WIFI_IPV4_MAX          16

typedef enum
{
    ECHOEAR_WIFI_SECURITY_OPEN = 0,
    ECHOEAR_WIFI_SECURITY_WPA2_PSK,
    ECHOEAR_WIFI_SECURITY_WPA3_SAE,
    ECHOEAR_WIFI_SECURITY_WPA2_WPA3,
    ECHOEAR_WIFI_SECURITY_UNKNOWN
} echoear_wifi_security_t;

typedef enum
{
    ECHOEAR_WIFI_IDLE = 0,
    ECHOEAR_WIFI_SCAN_REQUESTED,
    ECHOEAR_WIFI_SCANNING,
    ECHOEAR_WIFI_SCAN_COMPLETE,
    ECHOEAR_WIFI_CONNECT_REQUESTED,
    ECHOEAR_WIFI_CONNECTING,
    ECHOEAR_WIFI_CONNECTED,
    ECHOEAR_WIFI_DISCONNECTED,
    ECHOEAR_WIFI_ERROR
} echoear_wifi_state_t;

typedef enum
{
    ECHOEAR_WIFI_ERROR_NONE = 0,
    ECHOEAR_WIFI_ERROR_SCAN_FAILED,
    ECHOEAR_WIFI_ERROR_NETWORK_NOT_FOUND,
    ECHOEAR_WIFI_ERROR_PASSWORD_REQUIRED,
    ECHOEAR_WIFI_ERROR_AUTH_FAILED,
    ECHOEAR_WIFI_ERROR_DHCP_FAILED,
    ECHOEAR_WIFI_ERROR_TIMEOUT,
    ECHOEAR_WIFI_ERROR_UNSUPPORTED_SECURITY,
    ECHOEAR_WIFI_ERROR_INTERNAL
} echoear_wifi_error_t;

typedef struct
{
    char ssid[ECHOEAR_WIFI_SSID_MAX];
    char bssid[ECHOEAR_WIFI_BSSID_MAX];

    int16_t rssi;
    uint8_t channel;

    echoear_wifi_security_t security;
    bool hidden;
} echoear_wifi_network_t;

typedef struct
{
    echoear_wifi_state_t state;
    echoear_wifi_error_t error;

    bool scan_requested;
    bool connect_requested;
    bool disconnect_requested;

    echoear_wifi_network_t networks[
        ECHOEAR_WIFI_MAX_NETWORKS];

    size_t network_count;
    uint32_t scan_generation;

    char selected_ssid[ECHOEAR_WIFI_SSID_MAX];
    echoear_wifi_security_t selected_security;

    char password[ECHOEAR_WIFI_PASSWORD_MAX];
    bool password_present;

    char connected_ssid[ECHOEAR_WIFI_SSID_MAX];
    char ip_address[ECHOEAR_WIFI_IPV4_MAX];
    char gateway[ECHOEAR_WIFI_IPV4_MAX];
    char netmask[ECHOEAR_WIFI_IPV4_MAX];

    int16_t connected_rssi;
} echoear_wifi_manager_t;

void echoear_wifi_manager_init(void);
void echoear_wifi_manager_reset(void);

echoear_wifi_manager_t *echoear_wifi_manager_get(void);

bool echoear_wifi_manager_request_scan(void);
bool echoear_wifi_manager_take_scan_request(void);

void echoear_wifi_manager_begin_scan(void);
void echoear_wifi_manager_clear_scan_results(void);

bool echoear_wifi_manager_add_network(
    const char *ssid,
    const char *bssid,
    int16_t rssi,
    uint8_t channel,
    echoear_wifi_security_t security,
    bool hidden);

void echoear_wifi_manager_finish_scan(void);

const echoear_wifi_network_t *
echoear_wifi_manager_find_network(
    const char *ssid);

bool echoear_wifi_manager_request_connect(
    const char *ssid,
    echoear_wifi_security_t security,
    const char *password);

bool echoear_wifi_manager_take_connect_request(void);

void echoear_wifi_manager_mark_connecting(void);

void echoear_wifi_manager_mark_connected(
    const char *ssid,
    const char *ip_address,
    const char *gateway,
    const char *netmask,
    int16_t rssi);

void echoear_wifi_manager_mark_disconnected(void);

bool echoear_wifi_manager_request_disconnect(void);
bool echoear_wifi_manager_take_disconnect_request(void);

void echoear_wifi_manager_set_error(
    echoear_wifi_error_t error);

void echoear_wifi_manager_clear_error(void);
void echoear_wifi_manager_clear_credentials(void);

bool echoear_wifi_manager_parse_state(
    const char *value,
    echoear_wifi_state_t *state);

bool echoear_wifi_manager_parse_error(
    const char *value,
    echoear_wifi_error_t *error);

bool echoear_wifi_manager_parse_security(
    const char *value,
    echoear_wifi_security_t *security);

const char *echoear_wifi_manager_state_name(
    echoear_wifi_state_t state);

const char *echoear_wifi_manager_error_name(
    echoear_wifi_error_t error);

const char *echoear_wifi_manager_security_name(
    echoear_wifi_security_t security);

bool echoear_wifi_manager_is_connected(void);
bool echoear_wifi_manager_has_pending_action(void);

#endif
