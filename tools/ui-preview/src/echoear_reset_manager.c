#include "echoear_reset_manager.h"

#include <string.h>

static echoear_reset_manager_t reset_manager;

static void apply_action_policy(
    echoear_reset_action_t action)
{
    reset_manager.confirmation_required =
        echoear_reset_manager_action_requires_confirmation(
            action);

    reset_manager.erase_wifi_credentials = false;
    reset_manager.erase_device_settings = false;
    reset_manager.enter_provisioning = false;
    reset_manager.restart_required = false;

    switch (action)
    {
    case ECHOEAR_RESET_ACTION_FORGET_WIFI:
        reset_manager.erase_wifi_credentials = true;
        reset_manager.enter_provisioning = true;
        break;

    case ECHOEAR_RESET_ACTION_REPROVISION:
        /*
         * Keep the current credentials until the new setup succeeds.
         * Integration will set force_provisioning=1.
         */
        reset_manager.enter_provisioning = true;
        break;

    case ECHOEAR_RESET_ACTION_FACTORY_RESET:
        reset_manager.erase_wifi_credentials = true;
        reset_manager.erase_device_settings = true;
        reset_manager.enter_provisioning = true;
        reset_manager.restart_required = true;
        break;

    case ECHOEAR_RESET_ACTION_NONE:
    default:
        break;
    }
}

void echoear_reset_manager_init(void)
{
    memset(&reset_manager, 0, sizeof(reset_manager));

    reset_manager.action = ECHOEAR_RESET_ACTION_NONE;
    reset_manager.source = ECHOEAR_RESET_SOURCE_UNKNOWN;
    reset_manager.state = ECHOEAR_RESET_IDLE;
    reset_manager.error = ECHOEAR_RESET_ERROR_NONE;
}

void echoear_reset_manager_reset(void)
{
    uint32_t generation = reset_manager.generation;

    echoear_reset_manager_init();
    reset_manager.generation = generation;
}

echoear_reset_manager_t *echoear_reset_manager_get(void)
{
    return &reset_manager;
}

bool echoear_reset_manager_request(
    echoear_reset_action_t action,
    echoear_reset_source_t source)
{
    if (action == ECHOEAR_RESET_ACTION_NONE)
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_INVALID_ACTION);

        return false;
    }

    if (echoear_reset_manager_is_active())
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_NOT_ALLOWED);

        return false;
    }

    echoear_reset_manager_reset();

    reset_manager.action = action;
    reset_manager.source = source;
    reset_manager.error = ECHOEAR_RESET_ERROR_NONE;
    reset_manager.generation++;

    apply_action_policy(action);

    reset_manager.state =
        reset_manager.confirmation_required
            ? ECHOEAR_RESET_WAITING_CONFIRMATION
            : ECHOEAR_RESET_EXECUTE_REQUESTED;

    reset_manager.confirmed =
        !reset_manager.confirmation_required;

    reset_manager.execute_requested =
        !reset_manager.confirmation_required;

    return true;
}

bool echoear_reset_manager_confirm(void)
{
    if (reset_manager.action ==
        ECHOEAR_RESET_ACTION_NONE)
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_INVALID_ACTION);

        return false;
    }

    if (!reset_manager.confirmation_required)
    {
        reset_manager.confirmed = true;
        reset_manager.execute_requested = true;
        reset_manager.state =
            ECHOEAR_RESET_EXECUTE_REQUESTED;

        return true;
    }

    if (reset_manager.state !=
        ECHOEAR_RESET_WAITING_CONFIRMATION)
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_NOT_ALLOWED);

        return false;
    }

    reset_manager.confirmed = true;
    reset_manager.execute_requested = true;
    reset_manager.error = ECHOEAR_RESET_ERROR_NONE;
    reset_manager.state =
        ECHOEAR_RESET_EXECUTE_REQUESTED;

    return true;
}

bool echoear_reset_manager_cancel(void)
{
    if (!echoear_reset_manager_is_active())
    {
        return false;
    }

    if (reset_manager.state ==
            ECHOEAR_RESET_RUNNING ||
        reset_manager.state ==
            ECHOEAR_RESET_EXECUTE_REQUESTED)
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_NOT_ALLOWED);

        return false;
    }

    reset_manager.confirmed = false;
    reset_manager.execute_requested = false;
    reset_manager.error = ECHOEAR_RESET_ERROR_NONE;
    reset_manager.state = ECHOEAR_RESET_CANCELLED;

    return true;
}

bool echoear_reset_manager_take_execute_request(void)
{
    bool requested =
        reset_manager.execute_requested;

    reset_manager.execute_requested = false;

    return requested;
}

void echoear_reset_manager_mark_running(void)
{
    reset_manager.execute_requested = false;
    reset_manager.error = ECHOEAR_RESET_ERROR_NONE;
    reset_manager.state = ECHOEAR_RESET_RUNNING;
}

void echoear_reset_manager_mark_credentials_erased(void)
{
    reset_manager.credentials_erased = true;
}

void echoear_reset_manager_mark_settings_erased(void)
{
    reset_manager.settings_erased = true;
}

void echoear_reset_manager_mark_provisioning_applied(void)
{
    reset_manager.provisioning_applied = true;
}

void echoear_reset_manager_mark_completed(void)
{
    if (reset_manager.erase_wifi_credentials &&
        !reset_manager.credentials_erased)
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_DELETE_CREDENTIALS_FAILED);

        return;
    }

    if (reset_manager.erase_device_settings &&
        !reset_manager.settings_erased)
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_RESET_SETTINGS_FAILED);

        return;
    }

    if (reset_manager.enter_provisioning &&
        !reset_manager.provisioning_applied)
    {
        echoear_reset_manager_set_error(
            ECHOEAR_RESET_ERROR_APPLY_FAILED);

        return;
    }

    reset_manager.execute_requested = false;
    reset_manager.error = ECHOEAR_RESET_ERROR_NONE;
    reset_manager.state = ECHOEAR_RESET_COMPLETED;
}

void echoear_reset_manager_set_error(
    echoear_reset_error_t error)
{
    reset_manager.error = error;

    if (error == ECHOEAR_RESET_ERROR_NONE)
    {
        return;
    }

    reset_manager.execute_requested = false;
    reset_manager.state = ECHOEAR_RESET_ERROR;
}

void echoear_reset_manager_clear_error(void)
{
    reset_manager.error = ECHOEAR_RESET_ERROR_NONE;

    if (reset_manager.state == ECHOEAR_RESET_ERROR)
    {
        reset_manager.state = ECHOEAR_RESET_IDLE;
        reset_manager.action = ECHOEAR_RESET_ACTION_NONE;
        reset_manager.source = ECHOEAR_RESET_SOURCE_UNKNOWN;
    }
}

bool echoear_reset_manager_action_requires_confirmation(
    echoear_reset_action_t action)
{
    return action ==
               ECHOEAR_RESET_ACTION_FORGET_WIFI ||
           action ==
               ECHOEAR_RESET_ACTION_FACTORY_RESET;
}

bool echoear_reset_manager_is_active(void)
{
    return reset_manager.state ==
               ECHOEAR_RESET_REQUESTED ||
           reset_manager.state ==
               ECHOEAR_RESET_WAITING_CONFIRMATION ||
           reset_manager.state ==
               ECHOEAR_RESET_CONFIRMED ||
           reset_manager.state ==
               ECHOEAR_RESET_EXECUTE_REQUESTED ||
           reset_manager.state ==
               ECHOEAR_RESET_RUNNING;
}

bool echoear_reset_manager_is_terminal(void)
{
    return reset_manager.state ==
               ECHOEAR_RESET_COMPLETED ||
           reset_manager.state ==
               ECHOEAR_RESET_CANCELLED ||
           reset_manager.state ==
               ECHOEAR_RESET_ERROR;
}

bool echoear_reset_manager_parse_action(
    const char *value,
    echoear_reset_action_t *action)
{
    if (value == NULL || action == NULL)
    {
        return false;
    }

    if (strcmp(value, "none") == 0 ||
        value[0] == '\0')
        *action = ECHOEAR_RESET_ACTION_NONE;
    else if (strcmp(value, "forget_wifi") == 0)
        *action = ECHOEAR_RESET_ACTION_FORGET_WIFI;
    else if (strcmp(value, "reprovision") == 0)
        *action = ECHOEAR_RESET_ACTION_REPROVISION;
    else if (strcmp(value, "factory_reset") == 0)
        *action = ECHOEAR_RESET_ACTION_FACTORY_RESET;
    else
        return false;

    return true;
}

bool echoear_reset_manager_parse_source(
    const char *value,
    echoear_reset_source_t *source)
{
    if (value == NULL || source == NULL)
    {
        return false;
    }

    if (strcmp(value, "unknown") == 0 ||
        value[0] == '\0')
        *source = ECHOEAR_RESET_SOURCE_UNKNOWN;
    else if (strcmp(value, "settings") == 0)
        *source = ECHOEAR_RESET_SOURCE_SETTINGS;
    else if (strcmp(value, "long_press") == 0)
        *source = ECHOEAR_RESET_SOURCE_LONG_PRESS;
    else if (strcmp(value, "portal") == 0)
        *source = ECHOEAR_RESET_SOURCE_PORTAL;
    else if (strcmp(value, "remote") == 0)
        *source = ECHOEAR_RESET_SOURCE_REMOTE;
    else if (strcmp(value, "test") == 0)
        *source = ECHOEAR_RESET_SOURCE_TEST;
    else
        return false;

    return true;
}

bool echoear_reset_manager_parse_state(
    const char *value,
    echoear_reset_state_t *state)
{
    if (value == NULL || state == NULL)
    {
        return false;
    }

    if (strcmp(value, "idle") == 0)
        *state = ECHOEAR_RESET_IDLE;
    else if (strcmp(value, "requested") == 0)
        *state = ECHOEAR_RESET_REQUESTED;
    else if (strcmp(value, "waiting_confirmation") == 0)
        *state = ECHOEAR_RESET_WAITING_CONFIRMATION;
    else if (strcmp(value, "confirmed") == 0)
        *state = ECHOEAR_RESET_CONFIRMED;
    else if (strcmp(value, "execute_requested") == 0)
        *state = ECHOEAR_RESET_EXECUTE_REQUESTED;
    else if (strcmp(value, "running") == 0)
        *state = ECHOEAR_RESET_RUNNING;
    else if (strcmp(value, "completed") == 0)
        *state = ECHOEAR_RESET_COMPLETED;
    else if (strcmp(value, "cancelled") == 0)
        *state = ECHOEAR_RESET_CANCELLED;
    else if (strcmp(value, "error") == 0)
        *state = ECHOEAR_RESET_ERROR;
    else
        return false;

    return true;
}

bool echoear_reset_manager_parse_error(
    const char *value,
    echoear_reset_error_t *error)
{
    if (value == NULL || error == NULL)
    {
        return false;
    }

    if (strcmp(value, "none") == 0 ||
        value[0] == '\0')
        *error = ECHOEAR_RESET_ERROR_NONE;
    else if (strcmp(value, "invalid_action") == 0)
        *error = ECHOEAR_RESET_ERROR_INVALID_ACTION;
    else if (strcmp(value, "not_allowed") == 0)
        *error = ECHOEAR_RESET_ERROR_NOT_ALLOWED;
    else if (strcmp(value, "confirmation_required") == 0)
        *error = ECHOEAR_RESET_ERROR_CONFIRMATION_REQUIRED;
    else if (strcmp(value, "delete_credentials_failed") == 0)
        *error = ECHOEAR_RESET_ERROR_DELETE_CREDENTIALS_FAILED;
    else if (strcmp(value, "reset_settings_failed") == 0)
        *error = ECHOEAR_RESET_ERROR_RESET_SETTINGS_FAILED;
    else if (strcmp(value, "storage_write_failed") == 0)
        *error = ECHOEAR_RESET_ERROR_STORAGE_WRITE_FAILED;
    else if (strcmp(value, "apply_failed") == 0)
        *error = ECHOEAR_RESET_ERROR_APPLY_FAILED;
    else if (strcmp(value, "internal") == 0)
        *error = ECHOEAR_RESET_ERROR_INTERNAL;
    else
        return false;

    return true;
}

const char *echoear_reset_manager_action_name(
    echoear_reset_action_t action)
{
    switch (action)
    {
    case ECHOEAR_RESET_ACTION_FORGET_WIFI:
        return "forget_wifi";
    case ECHOEAR_RESET_ACTION_REPROVISION:
        return "reprovision";
    case ECHOEAR_RESET_ACTION_FACTORY_RESET:
        return "factory_reset";
    case ECHOEAR_RESET_ACTION_NONE:
    default:
        return "none";
    }
}

const char *echoear_reset_manager_source_name(
    echoear_reset_source_t source)
{
    switch (source)
    {
    case ECHOEAR_RESET_SOURCE_SETTINGS:
        return "settings";
    case ECHOEAR_RESET_SOURCE_LONG_PRESS:
        return "long_press";
    case ECHOEAR_RESET_SOURCE_PORTAL:
        return "portal";
    case ECHOEAR_RESET_SOURCE_REMOTE:
        return "remote";
    case ECHOEAR_RESET_SOURCE_TEST:
        return "test";
    case ECHOEAR_RESET_SOURCE_UNKNOWN:
    default:
        return "unknown";
    }
}

const char *echoear_reset_manager_state_name(
    echoear_reset_state_t state)
{
    switch (state)
    {
    case ECHOEAR_RESET_IDLE:
        return "idle";
    case ECHOEAR_RESET_REQUESTED:
        return "requested";
    case ECHOEAR_RESET_WAITING_CONFIRMATION:
        return "waiting_confirmation";
    case ECHOEAR_RESET_CONFIRMED:
        return "confirmed";
    case ECHOEAR_RESET_EXECUTE_REQUESTED:
        return "execute_requested";
    case ECHOEAR_RESET_RUNNING:
        return "running";
    case ECHOEAR_RESET_COMPLETED:
        return "completed";
    case ECHOEAR_RESET_CANCELLED:
        return "cancelled";
    case ECHOEAR_RESET_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

const char *echoear_reset_manager_error_name(
    echoear_reset_error_t error)
{
    switch (error)
    {
    case ECHOEAR_RESET_ERROR_NONE:
        return "none";
    case ECHOEAR_RESET_ERROR_INVALID_ACTION:
        return "invalid_action";
    case ECHOEAR_RESET_ERROR_NOT_ALLOWED:
        return "not_allowed";
    case ECHOEAR_RESET_ERROR_CONFIRMATION_REQUIRED:
        return "confirmation_required";
    case ECHOEAR_RESET_ERROR_DELETE_CREDENTIALS_FAILED:
        return "delete_credentials_failed";
    case ECHOEAR_RESET_ERROR_RESET_SETTINGS_FAILED:
        return "reset_settings_failed";
    case ECHOEAR_RESET_ERROR_STORAGE_WRITE_FAILED:
        return "storage_write_failed";
    case ECHOEAR_RESET_ERROR_APPLY_FAILED:
        return "apply_failed";
    case ECHOEAR_RESET_ERROR_INTERNAL:
    default:
        return "internal";
    }
}
