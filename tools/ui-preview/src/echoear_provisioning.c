#include "echoear_provisioning.h"

#include <stddef.h>
#include <string.h>

static echoear_provisioning_t provisioning;

static void copy_text(
    char *destination,
    size_t destination_size,
    const char *source)
{
    if (destination == NULL || destination_size == 0U)
    {
        return;
    }

    if (source == NULL)
    {
        destination[0] = '\0';
        return;
    }

    strncpy(destination, source, destination_size - 1U);
    destination[destination_size - 1U] = '\0';
}

void echoear_provisioning_init(void)
{
    memset(&provisioning, 0, sizeof(provisioning));

    provisioning.state =
        ECHOEAR_PROVISIONING_NOT_STARTED;

    provisioning.error =
        ECHOEAR_PROVISIONING_ERROR_NONE;

    provisioning.enabled = false;
    provisioning.setup_completed = false;
    provisioning.client_connected = false;
    provisioning.wifi_rssi = 0;

    copy_text(
        provisioning.ap_ssid,
        sizeof(provisioning.ap_ssid),
        "EchoEar-Setup");

    copy_text(
        provisioning.ip_address,
        sizeof(provisioning.ip_address),
        "192.168.4.1");
}

void echoear_provisioning_reset(void)
{
    echoear_provisioning_init();
}

echoear_provisioning_t *echoear_provisioning_get(void)
{
    return &provisioning;
}

void echoear_provisioning_set_state(
    echoear_provisioning_state_t state)
{
    provisioning.state = state;

    if (state != ECHOEAR_PROVISIONING_ERROR)
    {
        provisioning.error =
            ECHOEAR_PROVISIONING_ERROR_NONE;
    }
}

void echoear_provisioning_set_error(
    echoear_provisioning_error_t error)
{
    provisioning.error = error;

    if (error != ECHOEAR_PROVISIONING_ERROR_NONE)
    {
        provisioning.state =
            ECHOEAR_PROVISIONING_ERROR;
    }
}

void echoear_provisioning_set_enabled(
    bool enabled)
{
    provisioning.enabled = enabled;
}

void echoear_provisioning_set_setup_completed(
    bool completed)
{
    provisioning.setup_completed = completed;

    if (completed)
    {
        provisioning.state =
            ECHOEAR_PROVISIONING_COMPLETED;

        provisioning.error =
            ECHOEAR_PROVISIONING_ERROR_NONE;
    }
}

void echoear_provisioning_set_client_connected(
    bool connected)
{
    provisioning.client_connected = connected;
}

void echoear_provisioning_set_ap_ssid(
    const char *ssid)
{
    copy_text(
        provisioning.ap_ssid,
        sizeof(provisioning.ap_ssid),
        ssid);
}

void echoear_provisioning_set_target_ssid(
    const char *ssid)
{
    copy_text(
        provisioning.target_ssid,
        sizeof(provisioning.target_ssid),
        ssid);
}

void echoear_provisioning_set_ip_address(
    const char *ip_address)
{
    copy_text(
        provisioning.ip_address,
        sizeof(provisioning.ip_address),
        ip_address);
}

void echoear_provisioning_set_wifi_rssi(
    int16_t rssi)
{
    provisioning.wifi_rssi = rssi;
}

bool echoear_provisioning_parse_state(
    const char *value,
    echoear_provisioning_state_t *state)
{
    if (value == NULL || state == NULL)
    {
        return false;
    }

    if (strcmp(value, "not_started") == 0)
        *state = ECHOEAR_PROVISIONING_NOT_STARTED;
    else if (strcmp(value, "checking") == 0)
        *state = ECHOEAR_PROVISIONING_CHECKING;
    else if (strcmp(value, "ap_starting") == 0)
        *state = ECHOEAR_PROVISIONING_AP_STARTING;
    else if (strcmp(value, "ap_ready") == 0)
        *state = ECHOEAR_PROVISIONING_AP_READY;
    else if (strcmp(value, "client_connected") == 0)
        *state = ECHOEAR_PROVISIONING_CLIENT_CONNECTED;
    else if (strcmp(value, "scanning") == 0)
        *state = ECHOEAR_PROVISIONING_SCANNING;
    else if (strcmp(value, "wifi_connecting") == 0)
        *state = ECHOEAR_PROVISIONING_WIFI_CONNECTING;
    else if (strcmp(value, "wifi_connected") == 0)
        *state = ECHOEAR_PROVISIONING_WIFI_CONNECTED;
    else if (strcmp(value, "saving") == 0)
        *state = ECHOEAR_PROVISIONING_SAVING;
    else if (strcmp(value, "completed") == 0)
        *state = ECHOEAR_PROVISIONING_COMPLETED;
    else if (strcmp(value, "error") == 0)
        *state = ECHOEAR_PROVISIONING_ERROR;
    else
        return false;

    return true;
}

bool echoear_provisioning_parse_error(
    const char *value,
    echoear_provisioning_error_t *error)
{
    if (value == NULL || error == NULL)
    {
        return false;
    }

    if (strcmp(value, "none") == 0 || *value == '\0')
        *error = ECHOEAR_PROVISIONING_ERROR_NONE;
    else if (strcmp(value, "ap_start_failed") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_AP_START_FAILED;
    else if (strcmp(value, "scan_failed") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_SCAN_FAILED;
    else if (strcmp(value, "network_not_found") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_NETWORK_NOT_FOUND;
    else if (strcmp(value, "auth_failed") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_AUTH_FAILED;
    else if (strcmp(value, "dhcp_failed") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_DHCP_FAILED;
    else if (strcmp(value, "save_failed") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_SAVE_FAILED;
    else if (strcmp(value, "timeout") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_TIMEOUT;
    else if (strcmp(value, "unknown") == 0)
        *error = ECHOEAR_PROVISIONING_ERROR_UNKNOWN;
    else
        return false;

    return true;
}

const char *echoear_provisioning_state_name(
    echoear_provisioning_state_t state)
{
    switch (state)
    {
    case ECHOEAR_PROVISIONING_NOT_STARTED:
        return "not_started";
    case ECHOEAR_PROVISIONING_CHECKING:
        return "checking";
    case ECHOEAR_PROVISIONING_AP_STARTING:
        return "ap_starting";
    case ECHOEAR_PROVISIONING_AP_READY:
        return "ap_ready";
    case ECHOEAR_PROVISIONING_CLIENT_CONNECTED:
        return "client_connected";
    case ECHOEAR_PROVISIONING_SCANNING:
        return "scanning";
    case ECHOEAR_PROVISIONING_WIFI_CONNECTING:
        return "wifi_connecting";
    case ECHOEAR_PROVISIONING_WIFI_CONNECTED:
        return "wifi_connected";
    case ECHOEAR_PROVISIONING_SAVING:
        return "saving";
    case ECHOEAR_PROVISIONING_COMPLETED:
        return "completed";
    case ECHOEAR_PROVISIONING_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *echoear_provisioning_error_name(
    echoear_provisioning_error_t error)
{
    switch (error)
    {
    case ECHOEAR_PROVISIONING_ERROR_NONE:
        return "none";
    case ECHOEAR_PROVISIONING_ERROR_AP_START_FAILED:
        return "ap_start_failed";
    case ECHOEAR_PROVISIONING_ERROR_SCAN_FAILED:
        return "scan_failed";
    case ECHOEAR_PROVISIONING_ERROR_NETWORK_NOT_FOUND:
        return "network_not_found";
    case ECHOEAR_PROVISIONING_ERROR_AUTH_FAILED:
        return "auth_failed";
    case ECHOEAR_PROVISIONING_ERROR_DHCP_FAILED:
        return "dhcp_failed";
    case ECHOEAR_PROVISIONING_ERROR_SAVE_FAILED:
        return "save_failed";
    case ECHOEAR_PROVISIONING_ERROR_TIMEOUT:
        return "timeout";
    case ECHOEAR_PROVISIONING_ERROR_UNKNOWN:
    default:
        return "unknown";
    }
}

bool echoear_provisioning_is_active(void)
{
    return provisioning.state !=
               ECHOEAR_PROVISIONING_NOT_STARTED &&
           provisioning.state !=
               ECHOEAR_PROVISIONING_COMPLETED &&
           provisioning.state !=
               ECHOEAR_PROVISIONING_ERROR;
}

bool echoear_provisioning_is_terminal(void)
{
    return provisioning.state ==
               ECHOEAR_PROVISIONING_COMPLETED ||
           provisioning.state ==
               ECHOEAR_PROVISIONING_ERROR;
}
