#include "echoear_captive_portal.h"

#include <stddef.h>
#include <string.h>

static echoear_captive_portal_t portal;

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

void echoear_captive_portal_init(void)
{
    memset(&portal, 0, sizeof(portal));

    portal.state = ECHOEAR_PORTAL_STOPPED;
    portal.page = ECHOEAR_PORTAL_PAGE_WELCOME;
    portal.error = ECHOEAR_PORTAL_ERROR_NONE;
    portal.redirect_enabled = true;

    copy_text(
        portal.title,
        sizeof(portal.title),
        "EchoEar Setup");

    copy_text(
        portal.locale,
        sizeof(portal.locale),
        "th-TH");

    copy_text(
        portal.portal_url,
        sizeof(portal.portal_url),
        "http://192.168.4.1/");

    copy_text(
        portal.route,
        sizeof(portal.route),
        "/");

    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        "Select a Wi-Fi network");
}

void echoear_captive_portal_reset(void)
{
    echoear_captive_portal_clear_credentials();
    echoear_captive_portal_init();
}

echoear_captive_portal_t *echoear_captive_portal_get(void)
{
    return &portal;
}

void echoear_captive_portal_set_enabled(
    bool enabled)
{
    portal.enabled = enabled;

    if (enabled)
    {
        if (portal.state == ECHOEAR_PORTAL_STOPPED)
        {
            portal.state = ECHOEAR_PORTAL_STARTING;
            portal.page = ECHOEAR_PORTAL_PAGE_WELCOME;
        }

        return;
    }

    portal.scan_requested = false;
    portal.connect_requested = false;
    portal.error = ECHOEAR_PORTAL_ERROR_NONE;
    portal.state = ECHOEAR_PORTAL_STOPPED;
    portal.page = ECHOEAR_PORTAL_PAGE_WELCOME;

    echoear_captive_portal_clear_selection();
}

void echoear_captive_portal_set_redirect_enabled(
    bool enabled)
{
    portal.redirect_enabled = enabled;
}

void echoear_captive_portal_set_state(
    echoear_captive_portal_state_t state)
{
    portal.state = state;

    if (state != ECHOEAR_PORTAL_ERROR)
    {
        portal.error = ECHOEAR_PORTAL_ERROR_NONE;
    }
}

void echoear_captive_portal_set_page(
    echoear_captive_portal_page_t page)
{
    portal.page = page;
}

void echoear_captive_portal_set_error(
    echoear_captive_portal_error_t error)
{
    portal.error = error;

    if (error == ECHOEAR_PORTAL_ERROR_NONE)
    {
        return;
    }

    portal.state = ECHOEAR_PORTAL_ERROR;
    portal.page = ECHOEAR_PORTAL_PAGE_ERROR;

    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        echoear_captive_portal_error_name(error));
}

void echoear_captive_portal_set_title(
    const char *title)
{
    copy_text(portal.title, sizeof(portal.title), title);
}

void echoear_captive_portal_set_locale(
    const char *locale)
{
    copy_text(portal.locale, sizeof(portal.locale), locale);
}

void echoear_captive_portal_set_portal_url(
    const char *portal_url)
{
    copy_text(
        portal.portal_url,
        sizeof(portal.portal_url),
        portal_url);
}

void echoear_captive_portal_set_route(
    const char *route)
{
    copy_text(portal.route, sizeof(portal.route), route);
}

void echoear_captive_portal_set_status_message(
    const char *message)
{
    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        message);
}

void echoear_captive_portal_mark_request(
    const char *route)
{
    portal.request_count++;
    echoear_captive_portal_set_route(route);
}

bool echoear_captive_portal_request_scan(void)
{
    if (!portal.enabled)
    {
        echoear_captive_portal_set_error(
            ECHOEAR_PORTAL_ERROR_INVALID_REQUEST);
        return false;
    }

    portal.scan_requested = true;
    portal.state = ECHOEAR_PORTAL_READY;
    portal.page = ECHOEAR_PORTAL_PAGE_WIFI_LIST;

    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        "Scanning for Wi-Fi networks");

    return true;
}

bool echoear_captive_portal_take_scan_request(void)
{
    bool requested = portal.scan_requested;
    portal.scan_requested = false;
    return requested;
}

bool echoear_captive_portal_select_network(
    const char *ssid,
    bool secured)
{
    if (!portal.enabled ||
        ssid == NULL ||
        ssid[0] == '\0')
    {
        echoear_captive_portal_set_error(
            ECHOEAR_PORTAL_ERROR_NETWORK_REQUIRED);
        return false;
    }

    echoear_captive_portal_clear_credentials();

    copy_text(
        portal.selected_ssid,
        sizeof(portal.selected_ssid),
        ssid);

    portal.selected_network_secured = secured;
    portal.state = ECHOEAR_PORTAL_NETWORK_SELECTED;
    portal.page = ECHOEAR_PORTAL_PAGE_WIFI_CREDENTIALS;

    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        secured
            ? "Enter the Wi-Fi password"
            : "Confirm connection");

    return true;
}

bool echoear_captive_portal_submit_credentials(
    const char *password)
{
    size_t password_length = 0U;

    if (!portal.enabled ||
        portal.selected_ssid[0] == '\0')
    {
        echoear_captive_portal_set_error(
            ECHOEAR_PORTAL_ERROR_NETWORK_REQUIRED);
        return false;
    }

    if (password != NULL)
    {
        password_length = strlen(password);
    }

    if (portal.selected_network_secured &&
        password_length == 0U)
    {
        echoear_captive_portal_set_error(
            ECHOEAR_PORTAL_ERROR_PASSWORD_REQUIRED);
        return false;
    }

    if (password_length >= ECHOEAR_PORTAL_PASSWORD_MAX)
    {
        echoear_captive_portal_set_error(
            ECHOEAR_PORTAL_ERROR_INVALID_REQUEST);
        return false;
    }

    echoear_captive_portal_clear_credentials();

    if (password_length > 0U)
    {
        copy_text(
            portal.password,
            sizeof(portal.password),
            password);

        portal.password_present = true;
    }

    portal.connect_requested = true;
    portal.state = ECHOEAR_PORTAL_CREDENTIALS_RECEIVED;
    portal.page = ECHOEAR_PORTAL_PAGE_CONNECTING;

    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        "Connecting to Wi-Fi");

    return true;
}

bool echoear_captive_portal_take_connect_request(void)
{
    bool requested = portal.connect_requested;
    portal.connect_requested = false;
    return requested;
}

void echoear_captive_portal_mark_connecting(void)
{
    portal.state = ECHOEAR_PORTAL_CONNECTING;
    portal.page = ECHOEAR_PORTAL_PAGE_CONNECTING;
    portal.error = ECHOEAR_PORTAL_ERROR_NONE;

    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        "Connecting to Wi-Fi");
}

void echoear_captive_portal_mark_success(void)
{
    portal.state = ECHOEAR_PORTAL_SUCCESS;
    portal.page = ECHOEAR_PORTAL_PAGE_SUCCESS;
    portal.error = ECHOEAR_PORTAL_ERROR_NONE;

    copy_text(
        portal.status_message,
        sizeof(portal.status_message),
        "Wi-Fi connected");

    echoear_captive_portal_clear_credentials();
}

void echoear_captive_portal_clear_credentials(void)
{
    clear_secret(portal.password, sizeof(portal.password));
    portal.password_present = false;
}

void echoear_captive_portal_clear_selection(void)
{
    echoear_captive_portal_clear_credentials();

    portal.selected_ssid[0] = '\0';
    portal.selected_network_secured = false;
    portal.connect_requested = false;
}

bool echoear_captive_portal_parse_state(
    const char *value,
    echoear_captive_portal_state_t *state)
{
    if (value == NULL || state == NULL)
    {
        return false;
    }

    if (strcmp(value, "stopped") == 0)
        *state = ECHOEAR_PORTAL_STOPPED;
    else if (strcmp(value, "starting") == 0)
        *state = ECHOEAR_PORTAL_STARTING;
    else if (strcmp(value, "ready") == 0)
        *state = ECHOEAR_PORTAL_READY;
    else if (strcmp(value, "network_selected") == 0)
        *state = ECHOEAR_PORTAL_NETWORK_SELECTED;
    else if (strcmp(value, "credentials_received") == 0)
        *state = ECHOEAR_PORTAL_CREDENTIALS_RECEIVED;
    else if (strcmp(value, "connecting") == 0)
        *state = ECHOEAR_PORTAL_CONNECTING;
    else if (strcmp(value, "success") == 0)
        *state = ECHOEAR_PORTAL_SUCCESS;
    else if (strcmp(value, "error") == 0)
        *state = ECHOEAR_PORTAL_ERROR;
    else
        return false;

    return true;
}

bool echoear_captive_portal_parse_page(
    const char *value,
    echoear_captive_portal_page_t *page)
{
    if (value == NULL || page == NULL)
    {
        return false;
    }

    if (strcmp(value, "welcome") == 0)
        *page = ECHOEAR_PORTAL_PAGE_WELCOME;
    else if (strcmp(value, "wifi_list") == 0)
        *page = ECHOEAR_PORTAL_PAGE_WIFI_LIST;
    else if (strcmp(value, "wifi_credentials") == 0)
        *page = ECHOEAR_PORTAL_PAGE_WIFI_CREDENTIALS;
    else if (strcmp(value, "connecting") == 0)
        *page = ECHOEAR_PORTAL_PAGE_CONNECTING;
    else if (strcmp(value, "success") == 0)
        *page = ECHOEAR_PORTAL_PAGE_SUCCESS;
    else if (strcmp(value, "error") == 0)
        *page = ECHOEAR_PORTAL_PAGE_ERROR;
    else
        return false;

    return true;
}

bool echoear_captive_portal_parse_error(
    const char *value,
    echoear_captive_portal_error_t *error)
{
    if (value == NULL || error == NULL)
    {
        return false;
    }

    if (strcmp(value, "none") == 0 || value[0] == '\0')
        *error = ECHOEAR_PORTAL_ERROR_NONE;
    else if (strcmp(value, "invalid_request") == 0)
        *error = ECHOEAR_PORTAL_ERROR_INVALID_REQUEST;
    else if (strcmp(value, "network_required") == 0)
        *error = ECHOEAR_PORTAL_ERROR_NETWORK_REQUIRED;
    else if (strcmp(value, "password_required") == 0)
        *error = ECHOEAR_PORTAL_ERROR_PASSWORD_REQUIRED;
    else if (strcmp(value, "auth_failed") == 0)
        *error = ECHOEAR_PORTAL_ERROR_AUTH_FAILED;
    else if (strcmp(value, "network_not_found") == 0)
        *error = ECHOEAR_PORTAL_ERROR_NETWORK_NOT_FOUND;
    else if (strcmp(value, "timeout") == 0)
        *error = ECHOEAR_PORTAL_ERROR_TIMEOUT;
    else if (strcmp(value, "internal") == 0)
        *error = ECHOEAR_PORTAL_ERROR_INTERNAL;
    else
        return false;

    return true;
}

const char *echoear_captive_portal_state_name(
    echoear_captive_portal_state_t state)
{
    switch (state)
    {
    case ECHOEAR_PORTAL_STOPPED:
        return "stopped";
    case ECHOEAR_PORTAL_STARTING:
        return "starting";
    case ECHOEAR_PORTAL_READY:
        return "ready";
    case ECHOEAR_PORTAL_NETWORK_SELECTED:
        return "network_selected";
    case ECHOEAR_PORTAL_CREDENTIALS_RECEIVED:
        return "credentials_received";
    case ECHOEAR_PORTAL_CONNECTING:
        return "connecting";
    case ECHOEAR_PORTAL_SUCCESS:
        return "success";
    case ECHOEAR_PORTAL_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *echoear_captive_portal_page_name(
    echoear_captive_portal_page_t page)
{
    switch (page)
    {
    case ECHOEAR_PORTAL_PAGE_WELCOME:
        return "welcome";
    case ECHOEAR_PORTAL_PAGE_WIFI_LIST:
        return "wifi_list";
    case ECHOEAR_PORTAL_PAGE_WIFI_CREDENTIALS:
        return "wifi_credentials";
    case ECHOEAR_PORTAL_PAGE_CONNECTING:
        return "connecting";
    case ECHOEAR_PORTAL_PAGE_SUCCESS:
        return "success";
    case ECHOEAR_PORTAL_PAGE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *echoear_captive_portal_error_name(
    echoear_captive_portal_error_t error)
{
    switch (error)
    {
    case ECHOEAR_PORTAL_ERROR_NONE:
        return "none";
    case ECHOEAR_PORTAL_ERROR_INVALID_REQUEST:
        return "invalid_request";
    case ECHOEAR_PORTAL_ERROR_NETWORK_REQUIRED:
        return "network_required";
    case ECHOEAR_PORTAL_ERROR_PASSWORD_REQUIRED:
        return "password_required";
    case ECHOEAR_PORTAL_ERROR_AUTH_FAILED:
        return "auth_failed";
    case ECHOEAR_PORTAL_ERROR_NETWORK_NOT_FOUND:
        return "network_not_found";
    case ECHOEAR_PORTAL_ERROR_TIMEOUT:
        return "timeout";
    case ECHOEAR_PORTAL_ERROR_INTERNAL:
    default:
        return "internal";
    }
}

bool echoear_captive_portal_is_ready(void)
{
    return portal.enabled &&
           portal.state != ECHOEAR_PORTAL_STOPPED &&
           portal.state != ECHOEAR_PORTAL_ERROR;
}

bool echoear_captive_portal_has_pending_action(void)
{
    return portal.scan_requested ||
           portal.connect_requested;
}
