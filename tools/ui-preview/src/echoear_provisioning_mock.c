#include "echoear_provisioning_mock.h"

#include "echoear_provisioning.h"
#include "echoear_pro_ui.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim_whitespace(char *value)
{
    char *end;

    if (value == NULL)
    {
        return NULL;
    }

    while (*value != '\0' &&
           isspace((unsigned char)*value))
    {
        value++;
    }

    if (*value == '\0')
    {
        return value;
    }

    end = value + strlen(value) - 1;

    while (end > value &&
           isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    return value;
}

static bool parse_bool(const char *value)
{
    if (value == NULL)
    {
        return false;
    }

    return strcmp(value, "1") == 0 ||
           strcmp(value, "true") == 0 ||
           strcmp(value, "yes") == 0 ||
           strcmp(value, "on") == 0;
}

bool echoear_provisioning_mock_load(
    const char *path)
{
    FILE *file;
    char line[256];

    if (path == NULL)
    {
        return false;
    }

    file = fopen(path, "r");

    if (file == NULL)
    {
        return false;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *separator;
        char *key;
        char *value;

        key = trim_whitespace(line);

        if (key == NULL ||
            *key == '\0' ||
            *key == '#')
        {
            continue;
        }

        separator = strchr(key, '=');

        if (separator == NULL)
        {
            continue;
        }

        *separator = '\0';

        value = trim_whitespace(separator + 1);
        key = trim_whitespace(key);

        if (strcmp(key, "provisioning_enabled") == 0)
        {
            /*
             * auto preserves the decision made by
             * First Boot Detection.
             */
            if (strcmp(value, "auto") != 0)
            {
                echoear_provisioning_set_enabled(
                    parse_bool(value));
            }
        }
        else if (strcmp(key, "provisioning_state") == 0)
        {
            echoear_provisioning_state_t state;

            if (echoear_provisioning_parse_state(
                    value,
                    &state))
            {
                echoear_provisioning_set_state(state);
            }
        }
        else if (strcmp(key, "provisioning_error") == 0)
        {
            echoear_provisioning_error_t error;

            if (echoear_provisioning_parse_error(
                    value,
                    &error))
            {
                echoear_provisioning_set_error(error);
            }
        }
        else if (strcmp(key, "setup_completed") == 0)
        {
            /*
             * auto preserves persistent storage state.
             */
            if (strcmp(value, "auto") != 0)
            {
                echoear_provisioning_set_setup_completed(
                    parse_bool(value));
            }
        }
        else if (strcmp(key, "client_connected") == 0)
        {
            echoear_provisioning_set_client_connected(
                parse_bool(value));
        }
        else if (strcmp(key, "ap_ssid") == 0)
        {
            echoear_provisioning_set_ap_ssid(value);
        }
        else if (strcmp(key, "target_ssid") == 0)
        {
            echoear_provisioning_set_target_ssid(value);
        }
        else if (strcmp(key, "ip_address") == 0)
        {
            echoear_provisioning_set_ip_address(value);
        }
        else if (strcmp(key, "wifi_rssi") == 0)
        {
            echoear_provisioning_set_wifi_rssi(
                (int16_t)atoi(value));
        }
    }

    fclose(file);

    echoear_pro_ui_apply_provisioning_state();

    return true;
}