#include "echoear_provisioning_recovery.h"
#include <string.h>

static echoear_provisioning_recovery_t recovery;

static uint32_t calculate_delay(void)
{
    uint32_t delay = recovery.policy.retry_delay_ms;
    if (!recovery.policy.exponential_backoff || recovery.attempt <= 1U) return delay;
    for (uint8_t i = 1U; i < recovery.attempt; ++i) {
        if (delay >= recovery.policy.retry_delay_max_ms) return recovery.policy.retry_delay_max_ms;
        if (delay > recovery.policy.retry_delay_max_ms / 2U) return recovery.policy.retry_delay_max_ms;
        delay *= 2U;
    }
    return delay > recovery.policy.retry_delay_max_ms ? recovery.policy.retry_delay_max_ms : delay;
}

void echoear_provisioning_recovery_init(void)
{
    memset(&recovery, 0, sizeof(recovery));
    recovery.state = ECHOEAR_RECOVERY_STATE_IDLE;
}

void echoear_provisioning_recovery_reset(void)
{
    uint32_t generation = recovery.generation;
    echoear_provisioning_recovery_init();
    recovery.generation = generation;
}

echoear_provisioning_recovery_t *echoear_provisioning_recovery_get(void)
{
    return &recovery;
}

echoear_recovery_policy_t echoear_provisioning_recovery_default_policy(echoear_recovery_failure_t failure)
{
    echoear_recovery_policy_t p = {3U, 1000U, 8000U, true};
    switch (failure) {
        case ECHOEAR_RECOVERY_FAILURE_AUTH: p = (echoear_recovery_policy_t){1U,0U,0U,false}; break;
        case ECHOEAR_RECOVERY_FAILURE_NETWORK_NOT_FOUND: p = (echoear_recovery_policy_t){3U,1500U,6000U,true}; break;
        case ECHOEAR_RECOVERY_FAILURE_DHCP: p = (echoear_recovery_policy_t){3U,1000U,4000U,true}; break;
        case ECHOEAR_RECOVERY_FAILURE_WIFI_CONNECT_TIMEOUT: p = (echoear_recovery_policy_t){3U,1500U,6000U,true}; break;
        case ECHOEAR_RECOVERY_FAILURE_SOFTAP_START: p = (echoear_recovery_policy_t){3U,1000U,4000U,true}; break;
        case ECHOEAR_RECOVERY_FAILURE_PORTAL_IDLE_TIMEOUT: p = (echoear_recovery_policy_t){2U,1000U,2000U,false}; break;
        case ECHOEAR_RECOVERY_FAILURE_PROVISIONING_TIMEOUT: p = (echoear_recovery_policy_t){1U,0U,0U,false}; break;
        case ECHOEAR_RECOVERY_FAILURE_STORAGE: p = (echoear_recovery_policy_t){2U,500U,1000U,false}; break;
        default: break;
    }
    return p;
}

echoear_recovery_action_t echoear_provisioning_recovery_default_action(echoear_recovery_failure_t failure)
{
    switch (failure) {
        case ECHOEAR_RECOVERY_FAILURE_SOFTAP_START: return ECHOEAR_RECOVERY_ACTION_RESTART_SOFTAP;
        case ECHOEAR_RECOVERY_FAILURE_SCAN:
        case ECHOEAR_RECOVERY_FAILURE_NETWORK_NOT_FOUND: return ECHOEAR_RECOVERY_ACTION_RESCAN;
        case ECHOEAR_RECOVERY_FAILURE_DHCP:
        case ECHOEAR_RECOVERY_FAILURE_WIFI_CONNECT_TIMEOUT: return ECHOEAR_RECOVERY_ACTION_RECONNECT;
        case ECHOEAR_RECOVERY_FAILURE_PORTAL_IDLE_TIMEOUT: return ECHOEAR_RECOVERY_ACTION_REOPEN_PORTAL;
        case ECHOEAR_RECOVERY_FAILURE_PROVISIONING_TIMEOUT: return ECHOEAR_RECOVERY_ACTION_FALLBACK_PROVISIONING;
        case ECHOEAR_RECOVERY_FAILURE_AUTH: return ECHOEAR_RECOVERY_ACTION_REQUIRE_USER;
        case ECHOEAR_RECOVERY_FAILURE_STORAGE: return ECHOEAR_RECOVERY_ACTION_RETRY;
        case ECHOEAR_RECOVERY_FAILURE_INTERNAL: return ECHOEAR_RECOVERY_ACTION_FAIL_SAFE;
        default: return ECHOEAR_RECOVERY_ACTION_NONE;
    }
}

bool echoear_provisioning_recovery_report_failure(echoear_recovery_failure_t failure, uint32_t now_ms)
{
    if (failure == ECHOEAR_RECOVERY_FAILURE_NONE) return false;
    if (recovery.failure != failure || recovery.state == ECHOEAR_RECOVERY_STATE_IDLE || recovery.state == ECHOEAR_RECOVERY_STATE_RECOVERED) recovery.attempt = 0U;
    recovery.failure = failure;
    recovery.policy = echoear_provisioning_recovery_default_policy(failure);
    recovery.action = echoear_provisioning_recovery_default_action(failure);
    recovery.generation++;

    if (failure == ECHOEAR_RECOVERY_FAILURE_AUTH) {
        recovery.attempt = 1U;
        recovery.user_action_required = true;
        recovery.action_requested = true;
        recovery.state = ECHOEAR_RECOVERY_STATE_ACTION_REQUESTED;
        return true;
    }

    recovery.attempt++;
    if (recovery.attempt > recovery.policy.max_attempts) {
        recovery.retry_scheduled = false;
        recovery.action_requested = false;
        recovery.user_action_required = true;
        recovery.action = ECHOEAR_RECOVERY_ACTION_REQUIRE_USER;
        recovery.state = ECHOEAR_RECOVERY_STATE_EXHAUSTED;
        return true;
    }

    recovery.current_delay_ms = calculate_delay();
    recovery.retry_due_ms = now_ms + recovery.current_delay_ms;
    recovery.retry_scheduled = true;
    recovery.action_requested = false;
    recovery.user_action_required = false;
    recovery.state = ECHOEAR_RECOVERY_STATE_RETRY_WAIT;
    return true;
}

void echoear_provisioning_recovery_tick(uint32_t now_ms)
{
    if (recovery.state != ECHOEAR_RECOVERY_STATE_RETRY_WAIT || !recovery.retry_scheduled) return;
    if ((int32_t)(now_ms - recovery.retry_due_ms) < 0) return;
    recovery.retry_scheduled = false;
    recovery.action_requested = true;
    recovery.state = ECHOEAR_RECOVERY_STATE_ACTION_REQUESTED;
}

bool echoear_provisioning_recovery_take_action(echoear_recovery_action_t *action)
{
    if (!recovery.action_requested || action == NULL) return false;
    *action = recovery.action;
    recovery.action_requested = false;
    recovery.state = ECHOEAR_RECOVERY_STATE_WATCHING;
    return true;
}

void echoear_provisioning_recovery_mark_recovered(void)
{
    recovery.failure = ECHOEAR_RECOVERY_FAILURE_NONE;
    recovery.action = ECHOEAR_RECOVERY_ACTION_NONE;
    recovery.attempt = 0U;
    recovery.current_delay_ms = 0U;
    recovery.retry_due_ms = 0U;
    recovery.retry_scheduled = false;
    recovery.action_requested = false;
    recovery.user_action_required = false;
    recovery.generation++;
    recovery.state = ECHOEAR_RECOVERY_STATE_RECOVERED;
}

void echoear_provisioning_recovery_mark_action_failed(echoear_recovery_failure_t failure, uint32_t now_ms)
{
    echoear_provisioning_recovery_report_failure(failure == ECHOEAR_RECOVERY_FAILURE_NONE ? recovery.failure : failure, now_ms);
}

void echoear_provisioning_recovery_require_user(void)
{
    recovery.retry_scheduled = false;
    recovery.action_requested = true;
    recovery.user_action_required = true;
    recovery.action = ECHOEAR_RECOVERY_ACTION_REQUIRE_USER;
    recovery.state = ECHOEAR_RECOVERY_STATE_ACTION_REQUESTED;
}

void echoear_provisioning_recovery_set_error(void)
{
    recovery.retry_scheduled = false;
    recovery.action_requested = false;
    recovery.action = ECHOEAR_RECOVERY_ACTION_FAIL_SAFE;
    recovery.state = ECHOEAR_RECOVERY_STATE_ERROR;
}

bool echoear_provisioning_recovery_is_active(void)
{
    return recovery.state == ECHOEAR_RECOVERY_STATE_WATCHING || recovery.state == ECHOEAR_RECOVERY_STATE_RETRY_WAIT || recovery.state == ECHOEAR_RECOVERY_STATE_ACTION_REQUESTED;
}

bool echoear_provisioning_recovery_is_terminal(void)
{
    return recovery.state == ECHOEAR_RECOVERY_STATE_RECOVERED || recovery.state == ECHOEAR_RECOVERY_STATE_EXHAUSTED || recovery.state == ECHOEAR_RECOVERY_STATE_ERROR;
}

bool echoear_provisioning_recovery_parse_failure(const char *value, echoear_recovery_failure_t *failure)
{
    if (!value || !failure) return false;
    if (!strcmp(value,"none") || !*value) *failure=ECHOEAR_RECOVERY_FAILURE_NONE;
    else if (!strcmp(value,"softap_start")) *failure=ECHOEAR_RECOVERY_FAILURE_SOFTAP_START;
    else if (!strcmp(value,"scan")) *failure=ECHOEAR_RECOVERY_FAILURE_SCAN;
    else if (!strcmp(value,"network_not_found")) *failure=ECHOEAR_RECOVERY_FAILURE_NETWORK_NOT_FOUND;
    else if (!strcmp(value,"auth")) *failure=ECHOEAR_RECOVERY_FAILURE_AUTH;
    else if (!strcmp(value,"dhcp")) *failure=ECHOEAR_RECOVERY_FAILURE_DHCP;
    else if (!strcmp(value,"wifi_connect_timeout")) *failure=ECHOEAR_RECOVERY_FAILURE_WIFI_CONNECT_TIMEOUT;
    else if (!strcmp(value,"portal_idle_timeout")) *failure=ECHOEAR_RECOVERY_FAILURE_PORTAL_IDLE_TIMEOUT;
    else if (!strcmp(value,"provisioning_timeout")) *failure=ECHOEAR_RECOVERY_FAILURE_PROVISIONING_TIMEOUT;
    else if (!strcmp(value,"storage")) *failure=ECHOEAR_RECOVERY_FAILURE_STORAGE;
    else if (!strcmp(value,"internal")) *failure=ECHOEAR_RECOVERY_FAILURE_INTERNAL;
    else return false;
    return true;
}

const char *echoear_provisioning_recovery_state_name(echoear_recovery_state_t state)
{
    switch (state) {
        case ECHOEAR_RECOVERY_STATE_IDLE: return "idle";
        case ECHOEAR_RECOVERY_STATE_WATCHING: return "watching";
        case ECHOEAR_RECOVERY_STATE_RETRY_WAIT: return "retry_wait";
        case ECHOEAR_RECOVERY_STATE_ACTION_REQUESTED: return "action_requested";
        case ECHOEAR_RECOVERY_STATE_RECOVERED: return "recovered";
        case ECHOEAR_RECOVERY_STATE_EXHAUSTED: return "exhausted";
        default: return "error";
    }
}

const char *echoear_provisioning_recovery_failure_name(echoear_recovery_failure_t failure)
{
    switch (failure) {
        case ECHOEAR_RECOVERY_FAILURE_NONE: return "none";
        case ECHOEAR_RECOVERY_FAILURE_SOFTAP_START: return "softap_start";
        case ECHOEAR_RECOVERY_FAILURE_SCAN: return "scan";
        case ECHOEAR_RECOVERY_FAILURE_NETWORK_NOT_FOUND: return "network_not_found";
        case ECHOEAR_RECOVERY_FAILURE_AUTH: return "auth";
        case ECHOEAR_RECOVERY_FAILURE_DHCP: return "dhcp";
        case ECHOEAR_RECOVERY_FAILURE_WIFI_CONNECT_TIMEOUT: return "wifi_connect_timeout";
        case ECHOEAR_RECOVERY_FAILURE_PORTAL_IDLE_TIMEOUT: return "portal_idle_timeout";
        case ECHOEAR_RECOVERY_FAILURE_PROVISIONING_TIMEOUT: return "provisioning_timeout";
        case ECHOEAR_RECOVERY_FAILURE_STORAGE: return "storage";
        default: return "internal";
    }
}

const char *echoear_provisioning_recovery_action_name(echoear_recovery_action_t action)
{
    switch (action) {
        case ECHOEAR_RECOVERY_ACTION_NONE: return "none";
        case ECHOEAR_RECOVERY_ACTION_RETRY: return "retry";
        case ECHOEAR_RECOVERY_ACTION_RESCAN: return "rescan";
        case ECHOEAR_RECOVERY_ACTION_RECONNECT: return "reconnect";
        case ECHOEAR_RECOVERY_ACTION_RESTART_SOFTAP: return "restart_softap";
        case ECHOEAR_RECOVERY_ACTION_REOPEN_PORTAL: return "reopen_portal";
        case ECHOEAR_RECOVERY_ACTION_FALLBACK_PROVISIONING: return "fallback_provisioning";
        case ECHOEAR_RECOVERY_ACTION_REQUIRE_USER: return "require_user";
        default: return "fail_safe";
    }
}
