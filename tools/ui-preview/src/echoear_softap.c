#include "echoear_softap.h"

#include <stddef.h>
#include <string.h>

static echoear_softap_t softap;

static void copy_text(
    char *destination,
    size_t destination_size,
    const char *source)
{
    if (destination == NULL ||
        destination_size == 0U)
    {
        return;
    }

    if (source == NULL)
    {
        destination[0] = '\0';
        return;
    }

    strncpy(
        destination,
        source,
        destination_size - 1U);

    destination[destination_size - 1U] = '\0';
}

static bool configuration_is_valid(
    const char *ssid,
    const char *password,
    echoear_softap_auth_mode_t auth_mode)
{
    size_t password_length;

    if (ssid == NULL || ssid[0] == '\0')
    {
        return false;
    }

    if (strlen(ssid) >= ECHOEAR_SOFTAP_SSID_MAX)
    {
        return false;
    }

    if (auth_mode == ECHOEAR_SOFTAP_AUTH_OPEN)
    {
        return true;
    }

    if (password == NULL)
    {
        return false;
    }

    password_length = strlen(password);

    return password_length >= 8U &&
           password_length <= 63U;
}

void echoear_softap_init(void)
{
    memset(&softap, 0, sizeof(softap));

    softap.requested = false;

    softap.state =
        ECHOEAR_SOFTAP_STOPPED;

    softap.error =
        ECHOEAR_SOFTAP_ERROR_NONE;

    softap.auth_mode =
        ECHOEAR_SOFTAP_AUTH_OPEN;

    softap.channel =
        ECHOEAR_SOFTAP_DEFAULT_CHANNEL;

    softap.max_clients =
        ECHOEAR_SOFTAP_DEFAULT_MAX_CLIENTS;

    copy_text(
        softap.ssid,
        sizeof(softap.ssid),
        "EchoEar-Setup");

    copy_text(
        softap.ip_address,
        sizeof(softap.ip_address),
        "192.168.4.1");

    copy_text(
        softap.gateway,
        sizeof(softap.gateway),
        "192.168.4.1");

    copy_text(
        softap.netmask,
        sizeof(softap.netmask),
        "255.255.255.0");

    copy_text(
        softap.hostname,
        sizeof(softap.hostname),
        "echoear.local");
}

void echoear_softap_reset(void)
{
    echoear_softap_init();
}

echoear_softap_t *echoear_softap_get(void)
{
    return &softap;
}

bool echoear_softap_configure(
    const char *ssid,
    const char *password,
    echoear_softap_auth_mode_t auth_mode)
{
    if (!configuration_is_valid(
            ssid,
            password,
            auth_mode))
    {
        echoear_softap_set_error(
            ECHOEAR_SOFTAP_ERROR_INVALID_CONFIG);

        return false;
    }

    copy_text(
        softap.ssid,
        sizeof(softap.ssid),
        ssid);

    copy_text(
        softap.password,
        sizeof(softap.password),
        password);

    softap.auth_mode = auth_mode;
    softap.error = ECHOEAR_SOFTAP_ERROR_NONE;

    return true;
}

void echoear_softap_set_requested(
    bool requested)
{
    softap.requested = requested;
}

void echoear_softap_set_state(
    echoear_softap_state_t state)
{
    softap.state = state;

    if (state != ECHOEAR_SOFTAP_ERROR)
    {
        softap.error =
            ECHOEAR_SOFTAP_ERROR_NONE;
    }
}

void echoear_softap_set_error(
    echoear_softap_error_t error)
{
    softap.error = error;

    if (error != ECHOEAR_SOFTAP_ERROR_NONE)
    {
        softap.state =
            ECHOEAR_SOFTAP_ERROR;
    }
}

void echoear_softap_set_network(
    const char *ip_address,
    const char *gateway,
    const char *netmask)
{
    copy_text(
        softap.ip_address,
        sizeof(softap.ip_address),
        ip_address);

    copy_text(
        softap.gateway,
        sizeof(softap.gateway),
        gateway);

    copy_text(
        softap.netmask,
        sizeof(softap.netmask),
        netmask);
}

void echoear_softap_set_hostname(
    const char *hostname)
{
    copy_text(
        softap.hostname,
        sizeof(softap.hostname),
        hostname);
}

void echoear_softap_set_channel(
    uint8_t channel)
{
    if (channel < 1U)
    {
        channel = 1U;
    }

    if (channel > 13U)
    {
        channel = 13U;
    }

    softap.channel = channel;
}

void echoear_softap_set_max_clients(
    uint8_t max_clients)
{
    if (max_clients == 0U)
    {
        max_clients = 1U;
    }

    softap.max_clients = max_clients;

    if (softap.connected_clients >
        softap.max_clients)
    {
        softap.connected_clients =
            softap.max_clients;
    }
}

void echoear_softap_set_connected_clients(
    uint8_t connected_clients)
{
    if (connected_clients >
        softap.max_clients)
    {
        connected_clients =
            softap.max_clients;
    }

    softap.connected_clients =
        connected_clients;

    if (connected_clients > 0U &&
        softap.state == ECHOEAR_SOFTAP_ACTIVE)
    {
        softap.state =
            ECHOEAR_SOFTAP_CLIENT_CONNECTED;
    }
}

void echoear_softap_set_dns_redirect_ready(
    bool ready)
{
    softap.dns_redirect_ready = ready;
}

void echoear_softap_set_http_server_ready(
    bool ready)
{
    softap.http_server_ready = ready;
}

bool echoear_softap_parse_state(
    const char *value,
    echoear_softap_state_t *state)
{
    if (value == NULL || state == NULL)
    {
        return false;
    }

    if (strcmp(value, "stopped") == 0)
        *state = ECHOEAR_SOFTAP_STOPPED;
    else if (strcmp(value, "starting") == 0)
        *state = ECHOEAR_SOFTAP_STARTING;
    else if (strcmp(value, "active") == 0)
        *state = ECHOEAR_SOFTAP_ACTIVE;
    else if (strcmp(value, "client_connected") == 0)
        *state = ECHOEAR_SOFTAP_CLIENT_CONNECTED;
    else if (strcmp(value, "portal_ready") == 0)
        *state = ECHOEAR_SOFTAP_PORTAL_READY;
    else if (strcmp(value, "stopping") == 0)
        *state = ECHOEAR_SOFTAP_STOPPING;
    else if (strcmp(value, "error") == 0)
        *state = ECHOEAR_SOFTAP_ERROR;
    else
        return false;

    return true;
}

bool echoear_softap_parse_error(
    const char *value,
    echoear_softap_error_t *error)
{
    if (value == NULL || error == NULL)
    {
        return false;
    }

    if (strcmp(value, "none") == 0 ||
        value[0] == '\0')
        *error = ECHOEAR_SOFTAP_ERROR_NONE;
    else if (strcmp(value, "invalid_config") == 0)
        *error = ECHOEAR_SOFTAP_ERROR_INVALID_CONFIG;
    else if (strcmp(value, "start_failed") == 0)
        *error = ECHOEAR_SOFTAP_ERROR_START_FAILED;
    else if (strcmp(value, "dns_failed") == 0)
        *error = ECHOEAR_SOFTAP_ERROR_DNS_FAILED;
    else if (strcmp(value, "http_failed") == 0)
        *error = ECHOEAR_SOFTAP_ERROR_HTTP_FAILED;
    else if (strcmp(value, "timeout") == 0)
        *error = ECHOEAR_SOFTAP_ERROR_TIMEOUT;
    else if (strcmp(value, "unknown") == 0)
        *error = ECHOEAR_SOFTAP_ERROR_UNKNOWN;
    else
        return false;

    return true;
}

bool echoear_softap_parse_auth_mode(
    const char *value,
    echoear_softap_auth_mode_t *auth_mode)
{
    if (value == NULL || auth_mode == NULL)
    {
        return false;
    }

    if (strcmp(value, "open") == 0)
        *auth_mode = ECHOEAR_SOFTAP_AUTH_OPEN;
    else if (strcmp(value, "wpa2") == 0 ||
             strcmp(value, "wpa2_psk") == 0)
        *auth_mode = ECHOEAR_SOFTAP_AUTH_WPA2_PSK;
    else
        return false;

    return true;
}

const char *echoear_softap_state_name(
    echoear_softap_state_t state)
{
    switch (state)
    {
    case ECHOEAR_SOFTAP_STOPPED:
        return "stopped";
    case ECHOEAR_SOFTAP_STARTING:
        return "starting";
    case ECHOEAR_SOFTAP_ACTIVE:
        return "active";
    case ECHOEAR_SOFTAP_CLIENT_CONNECTED:
        return "client_connected";
    case ECHOEAR_SOFTAP_PORTAL_READY:
        return "portal_ready";
    case ECHOEAR_SOFTAP_STOPPING:
        return "stopping";
    case ECHOEAR_SOFTAP_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *echoear_softap_error_name(
    echoear_softap_error_t error)
{
    switch (error)
    {
    case ECHOEAR_SOFTAP_ERROR_NONE:
        return "none";
    case ECHOEAR_SOFTAP_ERROR_INVALID_CONFIG:
        return "invalid_config";
    case ECHOEAR_SOFTAP_ERROR_START_FAILED:
        return "start_failed";
    case ECHOEAR_SOFTAP_ERROR_DNS_FAILED:
        return "dns_failed";
    case ECHOEAR_SOFTAP_ERROR_HTTP_FAILED:
        return "http_failed";
    case ECHOEAR_SOFTAP_ERROR_TIMEOUT:
        return "timeout";
    case ECHOEAR_SOFTAP_ERROR_UNKNOWN:
    default:
        return "unknown";
    }
}

const char *echoear_softap_auth_mode_name(
    echoear_softap_auth_mode_t auth_mode)
{
    switch (auth_mode)
    {
    case ECHOEAR_SOFTAP_AUTH_WPA2_PSK:
        return "wpa2_psk";
    case ECHOEAR_SOFTAP_AUTH_OPEN:
    default:
        return "open";
    }
}

bool echoear_softap_is_active(void)
{
    return softap.state ==
               ECHOEAR_SOFTAP_ACTIVE ||
           softap.state ==
               ECHOEAR_SOFTAP_CLIENT_CONNECTED ||
           softap.state ==
               ECHOEAR_SOFTAP_PORTAL_READY;
}

bool echoear_softap_is_portal_ready(void)
{
    return echoear_softap_is_active() &&
           softap.dns_redirect_ready &&
           softap.http_server_ready;
}
