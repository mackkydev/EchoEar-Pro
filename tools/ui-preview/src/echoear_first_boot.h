#ifndef ECHOEAR_FIRST_BOOT_H
#define ECHOEAR_FIRST_BOOT_H

#include <stdbool.h>

typedef enum
{
    ECHOEAR_BOOT_MODE_UNKNOWN = 0,
    ECHOEAR_BOOT_MODE_PROVISIONING,
    ECHOEAR_BOOT_MODE_NORMAL
} echoear_boot_mode_t;

typedef enum
{
    ECHOEAR_BOOT_REASON_NONE = 0,
    ECHOEAR_BOOT_REASON_FIRST_BOOT,
    ECHOEAR_BOOT_REASON_WIFI_CREDENTIALS_MISSING,
    ECHOEAR_BOOT_REASON_FORCED_PROVISIONING
} echoear_boot_reason_t;

typedef struct
{
    bool setup_completed;
    bool wifi_credentials_saved;
    bool force_provisioning;

    echoear_boot_mode_t mode;
    echoear_boot_reason_t reason;
} echoear_first_boot_t;

void echoear_first_boot_init(void);

echoear_first_boot_t *echoear_first_boot_get(void);

void echoear_first_boot_set_setup_completed(
    bool completed);

void echoear_first_boot_set_wifi_credentials_saved(
    bool saved);

void echoear_first_boot_set_force_provisioning(
    bool forced);

void echoear_first_boot_evaluate(void);
void echoear_first_boot_apply(void);

bool echoear_first_boot_should_start_provisioning(void);

const char *echoear_first_boot_mode_name(
    echoear_boot_mode_t mode);

const char *echoear_first_boot_reason_name(
    echoear_boot_reason_t reason);

#endif