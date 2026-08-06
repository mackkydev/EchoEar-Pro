#ifndef ECHOEAR_CREDENTIALS_STORE_H
#define ECHOEAR_CREDENTIALS_STORE_H

#include "echoear_wifi_manager.h"

#include <stdbool.h>
#include <stdint.h>

#define ECHOEAR_CREDENTIALS_SSID_MAX      33
#define ECHOEAR_CREDENTIALS_PASSWORD_MAX  65

typedef enum
{
    ECHOEAR_CREDENTIALS_EMPTY = 0,
    ECHOEAR_CREDENTIALS_SAVE_REQUESTED,
    ECHOEAR_CREDENTIALS_SAVING,
    ECHOEAR_CREDENTIALS_VERIFYING,
    ECHOEAR_CREDENTIALS_SAVED,
    ECHOEAR_CREDENTIALS_DELETE_REQUESTED,
    ECHOEAR_CREDENTIALS_DELETING,
    ECHOEAR_CREDENTIALS_ERROR
} echoear_credentials_state_t;

typedef enum
{
    ECHOEAR_CREDENTIALS_ERROR_NONE = 0,
    ECHOEAR_CREDENTIALS_ERROR_INVALID_SSID,
    ECHOEAR_CREDENTIALS_ERROR_PASSWORD_REQUIRED,
    ECHOEAR_CREDENTIALS_ERROR_UNSUPPORTED_SECURITY,
    ECHOEAR_CREDENTIALS_ERROR_WRITE_FAILED,
    ECHOEAR_CREDENTIALS_ERROR_VERIFY_FAILED,
    ECHOEAR_CREDENTIALS_ERROR_DELETE_FAILED,
    ECHOEAR_CREDENTIALS_ERROR_INTERNAL
} echoear_credentials_error_t;

typedef struct
{
    echoear_credentials_state_t state;
    echoear_credentials_error_t error;

    bool save_requested;
    bool delete_requested;
    bool credentials_saved;
    bool password_present;

    char pending_ssid[ECHOEAR_CREDENTIALS_SSID_MAX];
    char pending_password[ECHOEAR_CREDENTIALS_PASSWORD_MAX];
    echoear_wifi_security_t pending_security;

    char saved_ssid[ECHOEAR_CREDENTIALS_SSID_MAX];
    echoear_wifi_security_t saved_security;

    uint32_t generation;
} echoear_credentials_store_t;

void echoear_credentials_store_init(void);
void echoear_credentials_store_reset(void);

echoear_credentials_store_t *echoear_credentials_store_get(void);

bool echoear_credentials_store_request_save(
    const char *ssid,
    echoear_wifi_security_t security,
    const char *password);

bool echoear_credentials_store_take_save_request(void);

void echoear_credentials_store_mark_saving(void);
void echoear_credentials_store_mark_verifying(void);

void echoear_credentials_store_mark_saved(
    const char *ssid,
    echoear_wifi_security_t security);

void echoear_credentials_store_load_metadata(
    bool credentials_saved,
    const char *ssid,
    echoear_wifi_security_t security);

bool echoear_credentials_store_request_delete(void);
bool echoear_credentials_store_take_delete_request(void);

void echoear_credentials_store_mark_deleting(void);
void echoear_credentials_store_mark_deleted(void);

void echoear_credentials_store_set_error(
    echoear_credentials_error_t error);

void echoear_credentials_store_clear_error(void);
void echoear_credentials_store_clear_pending_secret(void);
void echoear_credentials_store_clear_pending_request(void);

bool echoear_credentials_store_parse_state(
    const char *value,
    echoear_credentials_state_t *state);

bool echoear_credentials_store_parse_error(
    const char *value,
    echoear_credentials_error_t *error);

const char *echoear_credentials_store_state_name(
    echoear_credentials_state_t state);

const char *echoear_credentials_store_error_name(
    echoear_credentials_error_t error);

bool echoear_credentials_store_has_saved_credentials(void);
bool echoear_credentials_store_has_pending_action(void);

#endif
