#include "echoear_first_boot.h"

#include "echoear_provisioning.h"

#include <string.h>

static echoear_first_boot_t first_boot;

void echoear_first_boot_init(void)
{
    memset(&first_boot, 0, sizeof(first_boot));

    first_boot.mode =
        ECHOEAR_BOOT_MODE_UNKNOWN;

    first_boot.reason =
        ECHOEAR_BOOT_REASON_NONE;
}

echoear_first_boot_t *echoear_first_boot_get(void)
{
    return &first_boot;
}

void echoear_first_boot_set_setup_completed(
    bool completed)
{
    first_boot.setup_completed = completed;
}

void echoear_first_boot_set_wifi_credentials_saved(
    bool saved)
{
    first_boot.wifi_credentials_saved = saved;
}

void echoear_first_boot_set_force_provisioning(
    bool forced)
{
    first_boot.force_provisioning = forced;
}

void echoear_first_boot_evaluate(void)
{
    if (first_boot.force_provisioning)
    {
        first_boot.mode =
            ECHOEAR_BOOT_MODE_PROVISIONING;

        first_boot.reason =
            ECHOEAR_BOOT_REASON_FORCED_PROVISIONING;

        return;
    }

    if (!first_boot.setup_completed)
    {
        first_boot.mode =
            ECHOEAR_BOOT_MODE_PROVISIONING;

        first_boot.reason =
            ECHOEAR_BOOT_REASON_FIRST_BOOT;

        return;
    }

    if (!first_boot.wifi_credentials_saved)
    {
        first_boot.mode =
            ECHOEAR_BOOT_MODE_PROVISIONING;

        first_boot.reason =
            ECHOEAR_BOOT_REASON_WIFI_CREDENTIALS_MISSING;

        return;
    }

    first_boot.mode =
        ECHOEAR_BOOT_MODE_NORMAL;

    first_boot.reason =
        ECHOEAR_BOOT_REASON_NONE;
}

void echoear_first_boot_apply(void)
{
    echoear_first_boot_evaluate();

    if (first_boot.mode ==
        ECHOEAR_BOOT_MODE_PROVISIONING)
    {
        echoear_provisioning_set_enabled(true);

        echoear_provisioning_set_setup_completed(
            false);

        echoear_provisioning_set_state(
            ECHOEAR_PROVISIONING_CHECKING);

        return;
    }

    echoear_provisioning_set_setup_completed(true);
    echoear_provisioning_set_enabled(false);
}

bool echoear_first_boot_should_start_provisioning(void)
{
    return first_boot.mode ==
        ECHOEAR_BOOT_MODE_PROVISIONING;
}

const char *echoear_first_boot_mode_name(
    echoear_boot_mode_t mode)
{
    switch (mode)
    {
    case ECHOEAR_BOOT_MODE_PROVISIONING:
        return "provisioning";

    case ECHOEAR_BOOT_MODE_NORMAL:
        return "normal";

    case ECHOEAR_BOOT_MODE_UNKNOWN:
    default:
        return "unknown";
    }
}

const char *echoear_first_boot_reason_name(
    echoear_boot_reason_t reason)
{
    switch (reason)
    {
    case ECHOEAR_BOOT_REASON_FIRST_BOOT:
        return "first_boot";

    case ECHOEAR_BOOT_REASON_WIFI_CREDENTIALS_MISSING:
        return "wifi_credentials_missing";

    case ECHOEAR_BOOT_REASON_FORCED_PROVISIONING:
        return "forced_provisioning";

    case ECHOEAR_BOOT_REASON_NONE:
    default:
        return "none";
    }
}