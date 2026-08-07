#ifndef ECHOEAR_PROVISIONING_RECOVERY_H
#define ECHOEAR_PROVISIONING_RECOVERY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ECHOEAR_RECOVERY_STATE_IDLE = 0,
    ECHOEAR_RECOVERY_STATE_WATCHING,
    ECHOEAR_RECOVERY_STATE_RETRY_WAIT,
    ECHOEAR_RECOVERY_STATE_ACTION_REQUESTED,
    ECHOEAR_RECOVERY_STATE_RECOVERED,
    ECHOEAR_RECOVERY_STATE_EXHAUSTED,
    ECHOEAR_RECOVERY_STATE_ERROR
} echoear_recovery_state_t;

typedef enum {
    ECHOEAR_RECOVERY_FAILURE_NONE = 0,
    ECHOEAR_RECOVERY_FAILURE_SOFTAP_START,
    ECHOEAR_RECOVERY_FAILURE_SCAN,
    ECHOEAR_RECOVERY_FAILURE_NETWORK_NOT_FOUND,
    ECHOEAR_RECOVERY_FAILURE_AUTH,
    ECHOEAR_RECOVERY_FAILURE_DHCP,
    ECHOEAR_RECOVERY_FAILURE_WIFI_CONNECT_TIMEOUT,
    ECHOEAR_RECOVERY_FAILURE_PORTAL_IDLE_TIMEOUT,
    ECHOEAR_RECOVERY_FAILURE_PROVISIONING_TIMEOUT,
    ECHOEAR_RECOVERY_FAILURE_STORAGE,
    ECHOEAR_RECOVERY_FAILURE_INTERNAL
} echoear_recovery_failure_t;

typedef enum {
    ECHOEAR_RECOVERY_ACTION_NONE = 0,
    ECHOEAR_RECOVERY_ACTION_RETRY,
    ECHOEAR_RECOVERY_ACTION_RESCAN,
    ECHOEAR_RECOVERY_ACTION_RECONNECT,
    ECHOEAR_RECOVERY_ACTION_RESTART_SOFTAP,
    ECHOEAR_RECOVERY_ACTION_REOPEN_PORTAL,
    ECHOEAR_RECOVERY_ACTION_FALLBACK_PROVISIONING,
    ECHOEAR_RECOVERY_ACTION_REQUIRE_USER,
    ECHOEAR_RECOVERY_ACTION_FAIL_SAFE
} echoear_recovery_action_t;

typedef struct {
    uint8_t max_attempts;
    uint32_t retry_delay_ms;
    uint32_t retry_delay_max_ms;
    bool exponential_backoff;
} echoear_recovery_policy_t;

typedef struct {
    echoear_recovery_state_t state;
    echoear_recovery_failure_t failure;
    echoear_recovery_action_t action;
    echoear_recovery_policy_t policy;
    uint8_t attempt;
    uint32_t current_delay_ms;
    uint32_t retry_due_ms;
    bool retry_scheduled;
    bool action_requested;
    bool user_action_required;
    uint32_t generation;
} echoear_provisioning_recovery_t;

void echoear_provisioning_recovery_init(void);
void echoear_provisioning_recovery_reset(void);
echoear_provisioning_recovery_t *echoear_provisioning_recovery_get(void);

echoear_recovery_policy_t echoear_provisioning_recovery_default_policy(echoear_recovery_failure_t failure);
echoear_recovery_action_t echoear_provisioning_recovery_default_action(echoear_recovery_failure_t failure);

bool echoear_provisioning_recovery_report_failure(echoear_recovery_failure_t failure, uint32_t now_ms);
void echoear_provisioning_recovery_tick(uint32_t now_ms);
bool echoear_provisioning_recovery_take_action(echoear_recovery_action_t *action);
void echoear_provisioning_recovery_mark_recovered(void);
void echoear_provisioning_recovery_mark_action_failed(echoear_recovery_failure_t failure, uint32_t now_ms);
void echoear_provisioning_recovery_require_user(void);
void echoear_provisioning_recovery_set_error(void);

bool echoear_provisioning_recovery_is_active(void);
bool echoear_provisioning_recovery_is_terminal(void);

bool echoear_provisioning_recovery_parse_failure(const char *value, echoear_recovery_failure_t *failure);
const char *echoear_provisioning_recovery_state_name(echoear_recovery_state_t state);
const char *echoear_provisioning_recovery_failure_name(echoear_recovery_failure_t failure);
const char *echoear_provisioning_recovery_action_name(echoear_recovery_action_t action);

#endif
