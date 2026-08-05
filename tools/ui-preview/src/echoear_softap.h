#ifndef ECHOEAR_SOFTAP_H
#define ECHOEAR_SOFTAP_H

#include <stdbool.h>
#include <stdint.h>

#define ECHOEAR_SOFTAP_SSID_MAX       33
#define ECHOEAR_SOFTAP_PASSWORD_MAX   65
#define ECHOEAR_SOFTAP_IPV4_MAX       16
#define ECHOEAR_SOFTAP_HOSTNAME_MAX   64

#define ECHOEAR_SOFTAP_DEFAULT_CHANNEL      1
#define ECHOEAR_SOFTAP_DEFAULT_MAX_CLIENTS  4

typedef enum
{
    ECHOEAR_SOFTAP_AUTH_OPEN = 0,
    ECHOEAR_SOFTAP_AUTH_WPA2_PSK
} echoear_softap_auth_mode_t;

typedef enum
{
    ECHOEAR_SOFTAP_STOPPED = 0,
    ECHOEAR_SOFTAP_STARTING,
    ECHOEAR_SOFTAP_ACTIVE,
    ECHOEAR_SOFTAP_CLIENT_CONNECTED,
    ECHOEAR_SOFTAP_PORTAL_READY,
    ECHOEAR_SOFTAP_STOPPING,
    ECHOEAR_SOFTAP_ERROR
} echoear_softap_state_t;

typedef enum
{
    ECHOEAR_SOFTAP_ERROR_NONE = 0,
    ECHOEAR_SOFTAP_ERROR_INVALID_CONFIG,
    ECHOEAR_SOFTAP_ERROR_START_FAILED,
    ECHOEAR_SOFTAP_ERROR_DNS_FAILED,
    ECHOEAR_SOFTAP_ERROR_HTTP_FAILED,
    ECHOEAR_SOFTAP_ERROR_TIMEOUT,
    ECHOEAR_SOFTAP_ERROR_UNKNOWN
} echoear_softap_error_t;

typedef struct
{
    bool requested;
    bool dns_redirect_ready;
    bool http_server_ready;

    echoear_softap_state_t state;
    echoear_softap_error_t error;
    echoear_softap_auth_mode_t auth_mode;

    char ssid[ECHOEAR_SOFTAP_SSID_MAX];
    char password[ECHOEAR_SOFTAP_PASSWORD_MAX];

    char ip_address[ECHOEAR_SOFTAP_IPV4_MAX];
    char gateway[ECHOEAR_SOFTAP_IPV4_MAX];
    char netmask[ECHOEAR_SOFTAP_IPV4_MAX];
    char hostname[ECHOEAR_SOFTAP_HOSTNAME_MAX];

    uint8_t channel;
    uint8_t max_clients;
    uint8_t connected_clients;
} echoear_softap_t;

void echoear_softap_init(void);
void echoear_softap_reset(void);

echoear_softap_t *echoear_softap_get(void);

bool echoear_softap_configure(
    const char *ssid,
    const char *password,
    echoear_softap_auth_mode_t auth_mode);

void echoear_softap_set_requested(
    bool requested);

void echoear_softap_set_state(
    echoear_softap_state_t state);

void echoear_softap_set_error(
    echoear_softap_error_t error);

void echoear_softap_set_network(
    const char *ip_address,
    const char *gateway,
    const char *netmask);

void echoear_softap_set_hostname(
    const char *hostname);

void echoear_softap_set_channel(
    uint8_t channel);

void echoear_softap_set_max_clients(
    uint8_t max_clients);

void echoear_softap_set_connected_clients(
    uint8_t connected_clients);

void echoear_softap_set_dns_redirect_ready(
    bool ready);

void echoear_softap_set_http_server_ready(
    bool ready);

bool echoear_softap_parse_state(
    const char *value,
    echoear_softap_state_t *state);

bool echoear_softap_parse_error(
    const char *value,
    echoear_softap_error_t *error);

bool echoear_softap_parse_auth_mode(
    const char *value,
    echoear_softap_auth_mode_t *auth_mode);

const char *echoear_softap_state_name(
    echoear_softap_state_t state);

const char *echoear_softap_error_name(
    echoear_softap_error_t error);

const char *echoear_softap_auth_mode_name(
    echoear_softap_auth_mode_t auth_mode);

bool echoear_softap_is_active(void);
bool echoear_softap_is_portal_ready(void);

#endif
