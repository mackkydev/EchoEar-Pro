#ifndef ECHOEAR_RESET_MANAGER_H
#define ECHOEAR_RESET_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ECHOEAR_RESET_ACTION_NONE = 0,
    ECHOEAR_RESET_ACTION_FORGET_WIFI,
    ECHOEAR_RESET_ACTION_REPROVISION,
    ECHOEAR_RESET_ACTION_FACTORY_RESET
} echoear_reset_action_t;

typedef enum
{
    ECHOEAR_RESET_SOURCE_UNKNOWN = 0,
    ECHOEAR_RESET_SOURCE_SETTINGS,
    ECHOEAR_RESET_SOURCE_LONG_PRESS,
    ECHOEAR_RESET_SOURCE_PORTAL,
    ECHOEAR_RESET_SOURCE_REMOTE,
    ECHOEAR_RESET_SOURCE_TEST
} echoear_reset_source_t;

typedef enum
{
    ECHOEAR_RESET_IDLE = 0,
    ECHOEAR_RESET_REQUESTED,
    ECHOEAR_RESET_WAITING_CONFIRMATION,
    ECHOEAR_RESET_CONFIRMED,
    ECHOEAR_RESET_EXECUTE_REQUESTED,
    ECHOEAR_RESET_RUNNING,
    ECHOEAR_RESET_COMPLETED,
    ECHOEAR_RESET_CANCELLED,
    ECHOEAR_RESET_ERROR
} echoear_reset_state_t;

typedef enum
{
    ECHOEAR_RESET_ERROR_NONE = 0,
    ECHOEAR_RESET_ERROR_INVALID_ACTION,
    ECHOEAR_RESET_ERROR_NOT_ALLOWED,
    ECHOEAR_RESET_ERROR_CONFIRMATION_REQUIRED,
    ECHOEAR_RESET_ERROR_DELETE_CREDENTIALS_FAILED,
    ECHOEAR_RESET_ERROR_RESET_SETTINGS_FAILED,
    ECHOEAR_RESET_ERROR_STORAGE_WRITE_FAILED,
    ECHOEAR_RESET_ERROR_APPLY_FAILED,
    ECHOEAR_RESET_ERROR_INTERNAL
} echoear_reset_error_t;

typedef struct
{
    echoear_reset_action_t action;
    echoear_reset_source_t source;
    echoear_reset_state_t state;
    echoear_reset_error_t error;

    bool confirmation_required;
    bool confirmed;
    bool execute_requested;

    bool erase_wifi_credentials;
    bool erase_device_settings;
    bool enter_provisioning;
    bool restart_required;

    bool credentials_erased;
    bool settings_erased;
    bool provisioning_applied;

    uint32_t generation;
} echoear_reset_manager_t;

void echoear_reset_manager_init(void);
void echoear_reset_manager_reset(void);

echoear_reset_manager_t *echoear_reset_manager_get(void);

bool echoear_reset_manager_request(
    echoear_reset_action_t action,
    echoear_reset_source_t source);

bool echoear_reset_manager_confirm(void);
bool echoear_reset_manager_cancel(void);

bool echoear_reset_manager_take_execute_request(void);

void echoear_reset_manager_mark_running(void);
void echoear_reset_manager_mark_credentials_erased(void);
void echoear_reset_manager_mark_settings_erased(void);
void echoear_reset_manager_mark_provisioning_applied(void);
void echoear_reset_manager_mark_completed(void);

void echoear_reset_manager_set_error(
    echoear_reset_error_t error);

void echoear_reset_manager_clear_error(void);

bool echoear_reset_manager_action_requires_confirmation(
    echoear_reset_action_t action);

bool echoear_reset_manager_is_active(void);
bool echoear_reset_manager_is_terminal(void);

bool echoear_reset_manager_parse_action(
    const char *value,
    echoear_reset_action_t *action);

bool echoear_reset_manager_parse_source(
    const char *value,
    echoear_reset_source_t *source);

bool echoear_reset_manager_parse_state(
    const char *value,
    echoear_reset_state_t *state);

bool echoear_reset_manager_parse_error(
    const char *value,
    echoear_reset_error_t *error);

const char *echoear_reset_manager_action_name(
    echoear_reset_action_t action);

const char *echoear_reset_manager_source_name(
    echoear_reset_source_t source);

const char *echoear_reset_manager_state_name(
    echoear_reset_state_t state);

const char *echoear_reset_manager_error_name(
    echoear_reset_error_t error);

#endif
