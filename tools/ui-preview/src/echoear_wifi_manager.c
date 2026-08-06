#include "echoear_wifi_manager.h"

#include <stddef.h>
#include <string.h>

static echoear_wifi_manager_t manager;

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

static void clear_secret(
    char *value,
    size_t value_size)
{
    volatile char *cursor;

    if (value == NULL || value_size == 0U)
    {
        return;
    }

    cursor = (volatile char *)value;

    while (value_size > 0U)
    {
        *cursor = '\0';
        cursor++;
        value_size--;
    }
}

static bool security_requires_password(
    echoear_wifi_security_t security)
{
    return security != ECHOEAR_WIFI_SECURITY_OPEN;
}

static int find_network_index(
    const char *ssid)
{
    size_t index;

    if (ssid == NULL || ssid[0] == '\0')
    {
        return -1;
    }

    for (index = 0U;
         index < manager.network_count;
         index++)
    {
        if (strcmp(
                manager.networks[index].ssid,
                ssid) == 0)
        {
            return (int)index;
        }
    }

    return -1;
}

void echoear_wifi_manager_init(void)
{
    memset(&manager, 0, sizeof(manager));

    manager.state = ECHOEAR_WIFI_IDLE;
    manager.error = ECHOEAR_WIFI_ERROR_NONE;
    manager.selected_security =
        ECHOEAR_WIFI_SECURITY_UNKNOWN;
}

void echoear_wifi_manager_reset(void)
{
    echoear_wifi_manager_clear_credentials();
    echoear_wifi_manager_init();
}

echoear_wifi_manager_t *echoear_wifi_manager_get(void)
{
    return &manager;
}

bool echoear_wifi_manager_request_scan(void)
{
    if (manager.state == ECHOEAR_WIFI_CONNECTING)
    {
        return false;
    }

    manager.scan_requested = true;
    manager.error = ECHOEAR_WIFI_ERROR_NONE;
    manager.state = ECHOEAR_WIFI_SCAN_REQUESTED;

    return true;
}

bool echoear_wifi_manager_take_scan_request(void)
{
    bool requested = manager.scan_requested;
    manager.scan_requested = false;
    return requested;
}

void echoear_wifi_manager_begin_scan(void)
{
    manager.scan_requested = false;
    manager.error = ECHOEAR_WIFI_ERROR_NONE;
    manager.state = ECHOEAR_WIFI_SCANNING;

    echoear_wifi_manager_clear_scan_results();
}

void echoear_wifi_manager_clear_scan_results(void)
{
    memset(
        manager.networks,
        0,
        sizeof(manager.networks));

    manager.network_count = 0U;
}

bool echoear_wifi_manager_add_network(
    const char *ssid,
    const char *bssid,
    int16_t rssi,
    uint8_t channel,
    echoear_wifi_security_t security,
    bool hidden)
{
    echoear_wifi_network_t *network;
    int existing_index;

    if (ssid == NULL || ssid[0] == '\0')
    {
        return false;
    }

    existing_index = find_network_index(ssid);

    if (existing_index >= 0)
    {
        network = &manager.networks[
            (size_t)existing_index];

        if (rssi <= network->rssi)
        {
            return true;
        }
    }
    else
    {
        if (manager.network_count >=
            ECHOEAR_WIFI_MAX_NETWORKS)
        {
            return false;
        }

        network = &manager.networks[
            manager.network_count];

        manager.network_count++;
    }

    memset(network, 0, sizeof(*network));

    copy_text(
        network->ssid,
        sizeof(network->ssid),
        ssid);

    copy_text(
        network->bssid,
        sizeof(network->bssid),
        bssid);

    network->rssi = rssi;
    network->channel = channel;
    network->security = security;
    network->hidden = hidden;

    return true;
}

void echoear_wifi_manager_finish_scan(void)
{
    manager.scan_generation++;
    manager.error = ECHOEAR_WIFI_ERROR_NONE;
    manager.state = ECHOEAR_WIFI_SCAN_COMPLETE;
}

const echoear_wifi_network_t *
echoear_wifi_manager_find_network(
    const char *ssid)
{
    int index = find_network_index(ssid);

    if (index < 0)
    {
        return NULL;
    }

    return &manager.networks[(size_t)index];
}

bool echoear_wifi_manager_request_connect(
    const char *ssid,
    echoear_wifi_security_t security,
    const char *password)
{
    size_t password_length = 0U;

    if (ssid == NULL || ssid[0] == '\0')
    {
        echoear_wifi_manager_set_error(
            ECHOEAR_WIFI_ERROR_NETWORK_NOT_FOUND);
        return false;
    }

    if (security == ECHOEAR_WIFI_SECURITY_UNKNOWN)
    {
        echoear_wifi_manager_set_error(
            ECHOEAR_WIFI_ERROR_UNSUPPORTED_SECURITY);
        return false;
    }

    if (password != NULL)
    {
        password_length = strlen(password);
    }

    if (security_requires_password(security) &&
        password_length == 0U)
    {
        echoear_wifi_manager_set_error(
            ECHOEAR_WIFI_ERROR_PASSWORD_REQUIRED);
        return false;
    }

    if (password_length >= ECHOEAR_WIFI_PASSWORD_MAX)
    {
        echoear_wifi_manager_set_error(
            ECHOEAR_WIFI_ERROR_INTERNAL);
        return false;
    }

    echoear_wifi_manager_clear_credentials();

    copy_text(
        manager.selected_ssid,
        sizeof(manager.selected_ssid),
        ssid);

    manager.selected_security = security;

    if (password_length > 0U)
    {
        copy_text(
            manager.password,
            sizeof(manager.password),
            password);

        manager.password_present = true;
    }

    manager.connect_requested = true;
    manager.disconnect_requested = false;
    manager.error = ECHOEAR_WIFI_ERROR_NONE;
    manager.state = ECHOEAR_WIFI_CONNECT_REQUESTED;

    return true;
}

bool echoear_wifi_manager_take_connect_request(void)
{
    bool requested = manager.connect_requested;
    manager.connect_requested = false;
    return requested;
}

void echoear_wifi_manager_mark_connecting(void)
{
    manager.connect_requested = false;
    manager.error = ECHOEAR_WIFI_ERROR_NONE;
    manager.state = ECHOEAR_WIFI_CONNECTING;
}

void echoear_wifi_manager_mark_connected(
    const char *ssid,
    const char *ip_address,
    const char *gateway,
    const char *netmask,
    int16_t rssi)
{
    copy_text(
        manager.connected_ssid,
        sizeof(manager.connected_ssid),
        ssid);

    copy_text(
        manager.ip_address,
        sizeof(manager.ip_address),
        ip_address);

    copy_text(
        manager.gateway,
        sizeof(manager.gateway),
        gateway);

    copy_text(
        manager.netmask,
        sizeof(manager.netmask),
        netmask);

    manager.connected_rssi = rssi;
    manager.connect_requested = false;
    manager.disconnect_requested = false;
    manager.error = ECHOEAR_WIFI_ERROR_NONE;
    manager.state = ECHOEAR_WIFI_CONNECTED;

    echoear_wifi_manager_clear_credentials();
}

void echoear_wifi_manager_mark_disconnected(void)
{
    manager.connected_ssid[0] = '\0';
    manager.ip_address[0] = '\0';
    manager.gateway[0] = '\0';
    manager.netmask[0] = '\0';
    manager.connected_rssi = 0;

    manager.connect_requested = false;
    manager.disconnect_requested = false;
    manager.state = ECHOEAR_WIFI_DISCONNECTED;

    echoear_wifi_manager_clear_credentials();
}

bool echoear_wifi_manager_request_disconnect(void)
{
    if (!echoear_wifi_manager_is_connected())
    {
        return false;
    }

    manager.disconnect_requested = true;
    manager.connect_requested = false;

    return true;
}

bool echoear_wifi_manager_take_disconnect_request(void)
{
    bool requested = manager.disconnect_requested;
    manager.disconnect_requested = false;
    return requested;
}

void echoear_wifi_manager_set_error(
    echoear_wifi_error_t error)
{
    manager.error = error;

    if (error != ECHOEAR_WIFI_ERROR_NONE)
    {
        manager.connect_requested = false;
        manager.state = ECHOEAR_WIFI_ERROR;
        echoear_wifi_manager_clear_credentials();
    }
}

void echoear_wifi_manager_clear_error(void)
{
    manager.error = ECHOEAR_WIFI_ERROR_NONE;

    if (manager.state == ECHOEAR_WIFI_ERROR)
    {
        manager.state = ECHOEAR_WIFI_IDLE;
    }
}

void echoear_wifi_manager_clear_credentials(void)
{
    clear_secret(
        manager.password,
        sizeof(manager.password));

    manager.password_present = false;
}

bool echoear_wifi_manager_parse_state(
    const char *value,
    echoear_wifi_state_t *state)
{
    if (value == NULL || state == NULL)
    {
        return false;
    }

    if (strcmp(value, "idle") == 0)
        *state = ECHOEAR_WIFI_IDLE;
    else if (strcmp(value, "scan_requested") == 0)
        *state = ECHOEAR_WIFI_SCAN_REQUESTED;
    else if (strcmp(value, "scanning") == 0)
        *state = ECHOEAR_WIFI_SCANNING;
    else if (strcmp(value, "scan_complete") == 0)
        *state = ECHOEAR_WIFI_SCAN_COMPLETE;
    else if (strcmp(value, "connect_requested") == 0)
        *state = ECHOEAR_WIFI_CONNECT_REQUESTED;
    else if (strcmp(value, "connecting") == 0)
        *state = ECHOEAR_WIFI_CONNECTING;
    else if (strcmp(value, "connected") == 0)
        *state = ECHOEAR_WIFI_CONNECTED;
    else if (strcmp(value, "disconnected") == 0)
        *state = ECHOEAR_WIFI_DISCONNECTED;
    else if (strcmp(value, "error") == 0)
        *state = ECHOEAR_WIFI_ERROR;
    else
        return false;

    return true;
}

bool echoear_wifi_manager_parse_error(
    const char *value,
    echoear_wifi_error_t *error)
{
    if (value == NULL || error == NULL)
    {
        return false;
    }

    if (strcmp(value, "none") == 0 ||
        value[0] == '\0')
        *error = ECHOEAR_WIFI_ERROR_NONE;
    else if (strcmp(value, "scan_failed") == 0)
        *error = ECHOEAR_WIFI_ERROR_SCAN_FAILED;
    else if (strcmp(value, "network_not_found") == 0)
        *error = ECHOEAR_WIFI_ERROR_NETWORK_NOT_FOUND;
    else if (strcmp(value, "password_required") == 0)
        *error = ECHOEAR_WIFI_ERROR_PASSWORD_REQUIRED;
    else if (strcmp(value, "auth_failed") == 0)
        *error = ECHOEAR_WIFI_ERROR_AUTH_FAILED;
    else if (strcmp(value, "dhcp_failed") == 0)
        *error = ECHOEAR_WIFI_ERROR_DHCP_FAILED;
    else if (strcmp(value, "timeout") == 0)
        *error = ECHOEAR_WIFI_ERROR_TIMEOUT;
    else if (strcmp(value, "unsupported_security") == 0)
        *error = ECHOEAR_WIFI_ERROR_UNSUPPORTED_SECURITY;
    else if (strcmp(value, "internal") == 0)
        *error = ECHOEAR_WIFI_ERROR_INTERNAL;
    else
        return false;

    return true;
}

bool echoear_wifi_manager_parse_security(
    const char *value,
    echoear_wifi_security_t *security)
{
    if (value == NULL || security == NULL)
    {
        return false;
    }

    if (strcmp(value, "open") == 0)
        *security = ECHOEAR_WIFI_SECURITY_OPEN;
    else if (strcmp(value, "wpa2") == 0 ||
             strcmp(value, "wpa2_psk") == 0)
        *security = ECHOEAR_WIFI_SECURITY_WPA2_PSK;
    else if (strcmp(value, "wpa3") == 0 ||
             strcmp(value, "wpa3_sae") == 0)
        *security = ECHOEAR_WIFI_SECURITY_WPA3_SAE;
    else if (strcmp(value, "wpa2_wpa3") == 0)
        *security = ECHOEAR_WIFI_SECURITY_WPA2_WPA3;
    else if (strcmp(value, "unknown") == 0)
        *security = ECHOEAR_WIFI_SECURITY_UNKNOWN;
    else
        return false;

    return true;
}

const char *echoear_wifi_manager_state_name(
    echoear_wifi_state_t state)
{
    switch (state)
    {
    case ECHOEAR_WIFI_IDLE:
        return "idle";
    case ECHOEAR_WIFI_SCAN_REQUESTED:
        return "scan_requested";
    case ECHOEAR_WIFI_SCANNING:
        return "scanning";
    case ECHOEAR_WIFI_SCAN_COMPLETE:
        return "scan_complete";
    case ECHOEAR_WIFI_CONNECT_REQUESTED:
        return "connect_requested";
    case ECHOEAR_WIFI_CONNECTING:
        return "connecting";
    case ECHOEAR_WIFI_CONNECTED:
        return "connected";
    case ECHOEAR_WIFI_DISCONNECTED:
        return "disconnected";
    case ECHOEAR_WIFI_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *echoear_wifi_manager_error_name(
    echoear_wifi_error_t error)
{
    switch (error)
    {
    case ECHOEAR_WIFI_ERROR_NONE:
        return "none";
    case ECHOEAR_WIFI_ERROR_SCAN_FAILED:
        return "scan_failed";
    case ECHOEAR_WIFI_ERROR_NETWORK_NOT_FOUND:
        return "network_not_found";
    case ECHOEAR_WIFI_ERROR_PASSWORD_REQUIRED:
        return "password_required";
    case ECHOEAR_WIFI_ERROR_AUTH_FAILED:
        return "auth_failed";
    case ECHOEAR_WIFI_ERROR_DHCP_FAILED:
        return "dhcp_failed";
    case ECHOEAR_WIFI_ERROR_TIMEOUT:
        return "timeout";
    case ECHOEAR_WIFI_ERROR_UNSUPPORTED_SECURITY:
        return "unsupported_security";
    case ECHOEAR_WIFI_ERROR_INTERNAL:
    default:
        return "internal";
    }
}

const char *echoear_wifi_manager_security_name(
    echoear_wifi_security_t security)
{
    switch (security)
    {
    case ECHOEAR_WIFI_SECURITY_OPEN:
        return "open";
    case ECHOEAR_WIFI_SECURITY_WPA2_PSK:
        return "wpa2_psk";
    case ECHOEAR_WIFI_SECURITY_WPA3_SAE:
        return "wpa3_sae";
    case ECHOEAR_WIFI_SECURITY_WPA2_WPA3:
        return "wpa2_wpa3";
    case ECHOEAR_WIFI_SECURITY_UNKNOWN:
    default:
        return "unknown";
    }
}

bool echoear_wifi_manager_is_connected(void)
{
    return manager.state == ECHOEAR_WIFI_CONNECTED &&
           manager.connected_ssid[0] != '\0';
}

bool echoear_wifi_manager_has_pending_action(void)
{
    return manager.scan_requested ||
           manager.connect_requested ||
           manager.disconnect_requested;
}
