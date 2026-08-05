#ifndef ECHOEAR_CAPTIVE_PORTAL_H
#define ECHOEAR_CAPTIVE_PORTAL_H

#include <stdbool.h>
#include <stdint.h>

#define ECHOEAR_PORTAL_TITLE_MAX      64
#define ECHOEAR_PORTAL_LOCALE_MAX     16
#define ECHOEAR_PORTAL_URL_MAX        96
#define ECHOEAR_PORTAL_ROUTE_MAX      96
#define ECHOEAR_PORTAL_SSID_MAX       33
#define ECHOEAR_PORTAL_PASSWORD_MAX   65
#define ECHOEAR_PORTAL_MESSAGE_MAX    128

typedef enum
{
    ECHOEAR_PORTAL_STOPPED = 0,
    ECHOEAR_PORTAL_STARTING,
    ECHOEAR_PORTAL_READY,
    ECHOEAR_PORTAL_NETWORK_SELECTED,
    ECHOEAR_PORTAL_CREDENTIALS_RECEIVED,
    ECHOEAR_PORTAL_CONNECTING,
    ECHOEAR_PORTAL_SUCCESS,
    ECHOEAR_PORTAL_ERROR
} echoear_captive_portal_state_t;

typedef enum
{
    ECHOEAR_PORTAL_PAGE_WELCOME = 0,
    ECHOEAR_PORTAL_PAGE_WIFI_LIST,
    ECHOEAR_PORTAL_PAGE_WIFI_CREDENTIALS,
    ECHOEAR_PORTAL_PAGE_CONNECTING,
    ECHOEAR_PORTAL_PAGE_SUCCESS,
    ECHOEAR_PORTAL_PAGE_ERROR
} echoear_captive_portal_page_t;

typedef enum
{
    ECHOEAR_PORTAL_ERROR_NONE = 0,
    ECHOEAR_PORTAL_ERROR_INVALID_REQUEST,
    ECHOEAR_PORTAL_ERROR_NETWORK_REQUIRED,
    ECHOEAR_PORTAL_ERROR_PASSWORD_REQUIRED,
    ECHOEAR_PORTAL_ERROR_AUTH_FAILED,
    ECHOEAR_PORTAL_ERROR_NETWORK_NOT_FOUND,
    ECHOEAR_PORTAL_ERROR_TIMEOUT,
    ECHOEAR_PORTAL_ERROR_INTERNAL
} echoear_captive_portal_error_t;

typedef struct
{
    bool enabled;
    bool redirect_enabled;

    bool scan_requested;
    bool connect_requested;
    bool password_present;
    bool selected_network_secured;

    echoear_captive_portal_state_t state;
    echoear_captive_portal_page_t page;
    echoear_captive_portal_error_t error;

    char title[ECHOEAR_PORTAL_TITLE_MAX];
    char locale[ECHOEAR_PORTAL_LOCALE_MAX];
    char portal_url[ECHOEAR_PORTAL_URL_MAX];
    char route[ECHOEAR_PORTAL_ROUTE_MAX];

    char selected_ssid[ECHOEAR_PORTAL_SSID_MAX];
    char password[ECHOEAR_PORTAL_PASSWORD_MAX];
    char status_message[ECHOEAR_PORTAL_MESSAGE_MAX];

    uint32_t request_count;
} echoear_captive_portal_t;

void echoear_captive_portal_init(void);
void echoear_captive_portal_reset(void);

echoear_captive_portal_t *echoear_captive_portal_get(void);

void echoear_captive_portal_set_enabled(
    bool enabled);

void echoear_captive_portal_set_redirect_enabled(
    bool enabled);

void echoear_captive_portal_set_state(
    echoear_captive_portal_state_t state);

void echoear_captive_portal_set_page(
    echoear_captive_portal_page_t page);

void echoear_captive_portal_set_error(
    echoear_captive_portal_error_t error);

void echoear_captive_portal_set_title(
    const char *title);

void echoear_captive_portal_set_locale(
    const char *locale);

void echoear_captive_portal_set_portal_url(
    const char *portal_url);

void echoear_captive_portal_set_route(
    const char *route);

void echoear_captive_portal_set_status_message(
    const char *message);

void echoear_captive_portal_mark_request(
    const char *route);

bool echoear_captive_portal_request_scan(void);
bool echoear_captive_portal_take_scan_request(void);

bool echoear_captive_portal_select_network(
    const char *ssid,
    bool secured);

bool echoear_captive_portal_submit_credentials(
    const char *password);

bool echoear_captive_portal_take_connect_request(void);

void echoear_captive_portal_mark_connecting(void);
void echoear_captive_portal_mark_success(void);

void echoear_captive_portal_clear_credentials(void);
void echoear_captive_portal_clear_selection(void);

bool echoear_captive_portal_parse_state(
    const char *value,
    echoear_captive_portal_state_t *state);

bool echoear_captive_portal_parse_page(
    const char *value,
    echoear_captive_portal_page_t *page);

bool echoear_captive_portal_parse_error(
    const char *value,
    echoear_captive_portal_error_t *error);

const char *echoear_captive_portal_state_name(
    echoear_captive_portal_state_t state);

const char *echoear_captive_portal_page_name(
    echoear_captive_portal_page_t page);

const char *echoear_captive_portal_error_name(
    echoear_captive_portal_error_t error);

bool echoear_captive_portal_is_ready(void);
bool echoear_captive_portal_has_pending_action(void);

#endif
