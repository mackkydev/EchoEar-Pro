#include "echoear_credentials_store.h"

#include <stddef.h>
#include <string.h>

static echoear_credentials_store_t store;

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

void echoear_credentials_store_init(void)
{
    memset(&store, 0, sizeof(store));

    store.state = ECHOEAR_CREDENTIALS_EMPTY;
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    store.pending_security = ECHOEAR_WIFI_SECURITY_UNKNOWN;
    store.saved_security = ECHOEAR_WIFI_SECURITY_UNKNOWN;
}

void echoear_credentials_store_reset(void)
{
    echoear_credentials_store_clear_pending_secret();
    echoear_credentials_store_init();
}

echoear_credentials_store_t *echoear_credentials_store_get(void)
{
    return &store;
}

bool echoear_credentials_store_request_save(
    const char *ssid,
    echoear_wifi_security_t security,
    const char *password)
{
    size_t password_length = 0U;

    if (ssid == NULL || ssid[0] == '\0')
    {
        echoear_credentials_store_set_error(
            ECHOEAR_CREDENTIALS_ERROR_INVALID_SSID);
        return false;
    }

    if (security == ECHOEAR_WIFI_SECURITY_UNKNOWN)
    {
        echoear_credentials_store_set_error(
            ECHOEAR_CREDENTIALS_ERROR_UNSUPPORTED_SECURITY);
        return false;
    }

    if (password != NULL)
    {
        password_length = strlen(password);
    }

    if (security_requires_password(security) &&
        password_length == 0U)
    {
        echoear_credentials_store_set_error(
            ECHOEAR_CREDENTIALS_ERROR_PASSWORD_REQUIRED);
        return false;
    }

    if (password_length >= ECHOEAR_CREDENTIALS_PASSWORD_MAX)
    {
        echoear_credentials_store_set_error(
            ECHOEAR_CREDENTIALS_ERROR_INTERNAL);
        return false;
    }

    echoear_credentials_store_clear_pending_secret();

    copy_text(
        store.pending_ssid,
        sizeof(store.pending_ssid),
        ssid);

    store.pending_security = security;

    if (password_length > 0U)
    {
        copy_text(
            store.pending_password,
            sizeof(store.pending_password),
            password);

        store.password_present = true;
    }

    store.save_requested = true;
    store.delete_requested = false;
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    store.state = ECHOEAR_CREDENTIALS_SAVE_REQUESTED;

    return true;
}

bool echoear_credentials_store_take_save_request(void)
{
    bool requested = store.save_requested;
    store.save_requested = false;
    return requested;
}

void echoear_credentials_store_mark_saving(void)
{
    store.save_requested = false;
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    store.state = ECHOEAR_CREDENTIALS_SAVING;
}

void echoear_credentials_store_mark_verifying(void)
{
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    store.state = ECHOEAR_CREDENTIALS_VERIFYING;
}

void echoear_credentials_store_mark_saved(
    const char *ssid,
    echoear_wifi_security_t security)
{
    const char *saved_ssid = ssid;

    if (saved_ssid == NULL || saved_ssid[0] == '\0')
    {
        saved_ssid = store.pending_ssid;
    }

    if (security == ECHOEAR_WIFI_SECURITY_UNKNOWN)
    {
        security = store.pending_security;
    }

    copy_text(
        store.saved_ssid,
        sizeof(store.saved_ssid),
        saved_ssid);

    store.saved_security = security;
    store.credentials_saved = store.saved_ssid[0] != '\0';
    store.save_requested = false;
    store.delete_requested = false;
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;

    store.state = store.credentials_saved
        ? ECHOEAR_CREDENTIALS_SAVED
        : ECHOEAR_CREDENTIALS_EMPTY;

    if (store.credentials_saved)
    {
        store.generation++;
    }

    echoear_credentials_store_clear_pending_request();
}

void echoear_credentials_store_load_metadata(
    bool credentials_saved,
    const char *ssid,
    echoear_wifi_security_t security)
{
    store.credentials_saved = credentials_saved;

    copy_text(
        store.saved_ssid,
        sizeof(store.saved_ssid),
        credentials_saved ? ssid : NULL);

    store.saved_security = credentials_saved
        ? security
        : ECHOEAR_WIFI_SECURITY_UNKNOWN;

    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;

    store.state = credentials_saved
        ? ECHOEAR_CREDENTIALS_SAVED
        : ECHOEAR_CREDENTIALS_EMPTY;
}

bool echoear_credentials_store_request_delete(void)
{
    store.save_requested = false;
    store.delete_requested = true;
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    store.state = ECHOEAR_CREDENTIALS_DELETE_REQUESTED;

    echoear_credentials_store_clear_pending_secret();

    return true;
}

bool echoear_credentials_store_take_delete_request(void)
{
    bool requested = store.delete_requested;
    store.delete_requested = false;
    return requested;
}

void echoear_credentials_store_mark_deleting(void)
{
    store.delete_requested = false;
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    store.state = ECHOEAR_CREDENTIALS_DELETING;
}

void echoear_credentials_store_mark_deleted(void)
{
    echoear_credentials_store_clear_pending_request();

    store.saved_ssid[0] = '\0';
    store.saved_security = ECHOEAR_WIFI_SECURITY_UNKNOWN;
    store.credentials_saved = false;
    store.save_requested = false;
    store.delete_requested = false;
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    store.state = ECHOEAR_CREDENTIALS_EMPTY;
    store.generation++;
}

void echoear_credentials_store_set_error(
    echoear_credentials_error_t error)
{
    store.error = error;

    if (error == ECHOEAR_CREDENTIALS_ERROR_NONE)
    {
        return;
    }

    store.save_requested = false;
    store.delete_requested = false;
    store.state = ECHOEAR_CREDENTIALS_ERROR;

    echoear_credentials_store_clear_pending_secret();
}

void echoear_credentials_store_clear_error(void)
{
    store.error = ECHOEAR_CREDENTIALS_ERROR_NONE;

    if (store.state == ECHOEAR_CREDENTIALS_ERROR)
    {
        store.state = store.credentials_saved
            ? ECHOEAR_CREDENTIALS_SAVED
            : ECHOEAR_CREDENTIALS_EMPTY;
    }
}

void echoear_credentials_store_clear_pending_secret(void)
{
    clear_secret(
        store.pending_password,
        sizeof(store.pending_password));

    store.password_present = false;
}

void echoear_credentials_store_clear_pending_request(void)
{
    echoear_credentials_store_clear_pending_secret();

    store.pending_ssid[0] = '\0';
    store.pending_security = ECHOEAR_WIFI_SECURITY_UNKNOWN;
    store.save_requested = false;
}

bool echoear_credentials_store_parse_state(
    const char *value,
    echoear_credentials_state_t *state)
{
    if (value == NULL || state == NULL)
    {
        return false;
    }

    if (strcmp(value, "empty") == 0)
        *state = ECHOEAR_CREDENTIALS_EMPTY;
    else if (strcmp(value, "save_requested") == 0)
        *state = ECHOEAR_CREDENTIALS_SAVE_REQUESTED;
    else if (strcmp(value, "saving") == 0)
        *state = ECHOEAR_CREDENTIALS_SAVING;
    else if (strcmp(value, "verifying") == 0)
        *state = ECHOEAR_CREDENTIALS_VERIFYING;
    else if (strcmp(value, "saved") == 0)
        *state = ECHOEAR_CREDENTIALS_SAVED;
    else if (strcmp(value, "delete_requested") == 0)
        *state = ECHOEAR_CREDENTIALS_DELETE_REQUESTED;
    else if (strcmp(value, "deleting") == 0)
        *state = ECHOEAR_CREDENTIALS_DELETING;
    else if (strcmp(value, "error") == 0)
        *state = ECHOEAR_CREDENTIALS_ERROR;
    else
        return false;

    return true;
}

bool echoear_credentials_store_parse_error(
    const char *value,
    echoear_credentials_error_t *error)
{
    if (value == NULL || error == NULL)
    {
        return false;
    }

    if (strcmp(value, "none") == 0 || value[0] == '\0')
        *error = ECHOEAR_CREDENTIALS_ERROR_NONE;
    else if (strcmp(value, "invalid_ssid") == 0)
        *error = ECHOEAR_CREDENTIALS_ERROR_INVALID_SSID;
    else if (strcmp(value, "password_required") == 0)
        *error = ECHOEAR_CREDENTIALS_ERROR_PASSWORD_REQUIRED;
    else if (strcmp(value, "unsupported_security") == 0)
        *error = ECHOEAR_CREDENTIALS_ERROR_UNSUPPORTED_SECURITY;
    else if (strcmp(value, "write_failed") == 0)
        *error = ECHOEAR_CREDENTIALS_ERROR_WRITE_FAILED;
    else if (strcmp(value, "verify_failed") == 0)
        *error = ECHOEAR_CREDENTIALS_ERROR_VERIFY_FAILED;
    else if (strcmp(value, "delete_failed") == 0)
        *error = ECHOEAR_CREDENTIALS_ERROR_DELETE_FAILED;
    else if (strcmp(value, "internal") == 0)
        *error = ECHOEAR_CREDENTIALS_ERROR_INTERNAL;
    else
        return false;

    return true;
}

const char *echoear_credentials_store_state_name(
    echoear_credentials_state_t state)
{
    switch (state)
    {
    case ECHOEAR_CREDENTIALS_EMPTY:
        return "empty";
    case ECHOEAR_CREDENTIALS_SAVE_REQUESTED:
        return "save_requested";
    case ECHOEAR_CREDENTIALS_SAVING:
        return "saving";
    case ECHOEAR_CREDENTIALS_VERIFYING:
        return "verifying";
    case ECHOEAR_CREDENTIALS_SAVED:
        return "saved";
    case ECHOEAR_CREDENTIALS_DELETE_REQUESTED:
        return "delete_requested";
    case ECHOEAR_CREDENTIALS_DELETING:
        return "deleting";
    case ECHOEAR_CREDENTIALS_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *echoear_credentials_store_error_name(
    echoear_credentials_error_t error)
{
    switch (error)
    {
    case ECHOEAR_CREDENTIALS_ERROR_NONE:
        return "none";
    case ECHOEAR_CREDENTIALS_ERROR_INVALID_SSID:
        return "invalid_ssid";
    case ECHOEAR_CREDENTIALS_ERROR_PASSWORD_REQUIRED:
        return "password_required";
    case ECHOEAR_CREDENTIALS_ERROR_UNSUPPORTED_SECURITY:
        return "unsupported_security";
    case ECHOEAR_CREDENTIALS_ERROR_WRITE_FAILED:
        return "write_failed";
    case ECHOEAR_CREDENTIALS_ERROR_VERIFY_FAILED:
        return "verify_failed";
    case ECHOEAR_CREDENTIALS_ERROR_DELETE_FAILED:
        return "delete_failed";
    case ECHOEAR_CREDENTIALS_ERROR_INTERNAL:
    default:
        return "internal";
    }
}

bool echoear_credentials_store_has_saved_credentials(void)
{
    return store.credentials_saved &&
           store.saved_ssid[0] != '\0';
}

bool echoear_credentials_store_has_pending_action(void)
{
    return store.save_requested ||
           store.delete_requested;
}
