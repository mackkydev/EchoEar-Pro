#include "echoear_runtime_reconnect.h"

#include <string.h>

static echoear_runtime_reconnect_t reconnect_state;

static uint32_t fast_delay(uint8_t attempt)
{
    uint32_t delay =
        reconnect_state.policy.fast_retry_base_ms;

    if (!reconnect_state.policy.exponential_backoff ||
        attempt <= 1U)
        return delay;

    for (uint8_t i = 1U; i < attempt; i++) {
        if (delay >= reconnect_state.policy.fast_retry_max_ms)
            return reconnect_state.policy.fast_retry_max_ms;

        if (delay > reconnect_state.policy.fast_retry_max_ms / 2U)
            return reconnect_state.policy.fast_retry_max_ms;

        delay *= 2U;
    }

    if (delay > reconnect_state.policy.fast_retry_max_ms)
        delay = reconnect_state.policy.fast_retry_max_ms;

    return delay;
}

static void schedule_fast(uint32_t now_ms)
{
    reconnect_state.retry_delay_ms =
        fast_delay(reconnect_state.fast_attempt);

    reconnect_state.retry_due_ms =
        now_ms + reconnect_state.retry_delay_ms;

    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_RETRY_WAIT;

    reconnect_state.generation++;
}

static void schedule_offline(uint32_t now_ms)
{
    reconnect_state.retry_delay_ms =
        reconnect_state.policy.offline_retry_interval_ms;

    reconnect_state.retry_due_ms =
        now_ms + reconnect_state.retry_delay_ms;

    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_OFFLINE_WAIT;

    reconnect_state.generation++;
}

void echoear_runtime_reconnect_init(void)
{
    memset(&reconnect_state, 0, sizeof(reconnect_state));

    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_IDLE;

    reconnect_state.disconnect_reason =
        ECHOEAR_RUNTIME_DISCONNECT_NONE;

    reconnect_state.error =
        ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE;

    reconnect_state.policy =
        echoear_runtime_reconnect_default_policy();

    /* Runtime failure must never force provisioning by itself. */
    reconnect_state.provisioning_allowed = false;
}

void echoear_runtime_reconnect_reset(void)
{
    uint32_t generation = reconnect_state.generation;
    echoear_runtime_reconnect_init();
    reconnect_state.generation = generation;
}

echoear_runtime_reconnect_t *
echoear_runtime_reconnect_get(void)
{
    return &reconnect_state;
}

echoear_runtime_reconnect_policy_t
echoear_runtime_reconnect_default_policy(void)
{
    echoear_runtime_reconnect_policy_t policy;

    policy.fast_attempts = 3U;
    policy.fast_retry_base_ms = 1000U;
    policy.fast_retry_max_ms = 4000U;
    policy.offline_retry_interval_ms = 30000U;
    policy.exponential_backoff = true;

    return policy;
}

void echoear_runtime_reconnect_mark_network_ready(void)
{
    reconnect_state.network_ready = true;
    reconnect_state.reconnect_requested = false;
    reconnect_state.user_disconnect = false;

    reconnect_state.fast_attempt = 0U;
    reconnect_state.retry_delay_ms = 0U;
    reconnect_state.retry_due_ms = 0U;

    reconnect_state.disconnect_reason =
        ECHOEAR_RUNTIME_DISCONNECT_NONE;

    reconnect_state.error =
        ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE;

    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_MONITORING;

    reconnect_state.generation++;
}

bool echoear_runtime_reconnect_report_disconnect(
    echoear_runtime_disconnect_reason_t reason,
    uint32_t now_ms)
{
    if (reconnect_state.state ==
        ECHOEAR_RUNTIME_RECONNECT_STATE_SUSPENDED) {
        reconnect_state.error =
            ECHOEAR_RUNTIME_RECONNECT_ERROR_NOT_MONITORING;
        reconnect_state.generation++;
        return false;
    }

    reconnect_state.network_ready = false;
    reconnect_state.reconnect_requested = false;
    reconnect_state.disconnect_reason = reason;
    reconnect_state.disconnect_count++;
    reconnect_state.error =
        ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE;

    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_DISCONNECTED;

    reconnect_state.generation++;

    if (reason == ECHOEAR_RUNTIME_DISCONNECT_MANUAL) {
        reconnect_state.user_disconnect = true;
        reconnect_state.state =
            ECHOEAR_RUNTIME_RECONNECT_STATE_SUSPENDED;
        reconnect_state.generation++;
        return true;
    }

    reconnect_state.user_disconnect = false;
    reconnect_state.fast_attempt = 1U;
    schedule_fast(now_ms);

    return true;
}

void echoear_runtime_reconnect_tick(uint32_t now_ms)
{
    if (reconnect_state.state !=
            ECHOEAR_RUNTIME_RECONNECT_STATE_RETRY_WAIT &&
        reconnect_state.state !=
            ECHOEAR_RUNTIME_RECONNECT_STATE_OFFLINE_WAIT)
        return;

    if ((int32_t)(now_ms - reconnect_state.retry_due_ms) < 0)
        return;

    reconnect_state.reconnect_requested = true;
    reconnect_state.retry_delay_ms = 0U;
    reconnect_state.retry_due_ms = 0U;
    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECT_REQUESTED;
    reconnect_state.generation++;
}

bool echoear_runtime_reconnect_take_request(void)
{
    bool requested = reconnect_state.reconnect_requested;
    reconnect_state.reconnect_requested = false;
    return requested;
}

void echoear_runtime_reconnect_mark_connecting(void)
{
    reconnect_state.reconnect_requested = false;
    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECTING;
    reconnect_state.generation++;
}

void echoear_runtime_reconnect_mark_restored(void)
{
    reconnect_state.network_ready = true;
    reconnect_state.reconnect_requested = false;
    reconnect_state.user_disconnect = false;
    reconnect_state.retry_delay_ms = 0U;
    reconnect_state.retry_due_ms = 0U;
    reconnect_state.reconnect_count++;
    reconnect_state.error =
        ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE;
    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_RESTORED;
    reconnect_state.generation++;
}

void echoear_runtime_reconnect_mark_failed(uint32_t now_ms)
{
    reconnect_state.network_ready = false;
    reconnect_state.error =
        ECHOEAR_RUNTIME_RECONNECT_ERROR_RECONNECT_FAILED;

    if (reconnect_state.fast_attempt <
        reconnect_state.policy.fast_attempts) {
        reconnect_state.fast_attempt++;
        schedule_fast(now_ms);
        return;
    }

    schedule_offline(now_ms);
}

void echoear_runtime_reconnect_suspend(void)
{
    reconnect_state.reconnect_requested = false;
    reconnect_state.retry_delay_ms = 0U;
    reconnect_state.retry_due_ms = 0U;
    reconnect_state.state =
        ECHOEAR_RUNTIME_RECONNECT_STATE_SUSPENDED;
    reconnect_state.generation++;
}

void echoear_runtime_reconnect_resume(
    bool network_ready,
    uint32_t now_ms)
{
    reconnect_state.user_disconnect = false;
    reconnect_state.error =
        ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE;

    if (network_ready) {
        echoear_runtime_reconnect_mark_network_ready();
        return;
    }

    reconnect_state.network_ready = false;
    reconnect_state.fast_attempt = 1U;
    schedule_fast(now_ms);
}

void echoear_runtime_reconnect_clear_error(void)
{
    reconnect_state.error =
        ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE;
    reconnect_state.generation++;
}

bool echoear_runtime_reconnect_is_active(void)
{
    return reconnect_state.state !=
               ECHOEAR_RUNTIME_RECONNECT_STATE_IDLE &&
           reconnect_state.state !=
               ECHOEAR_RUNTIME_RECONNECT_STATE_SUSPENDED &&
           reconnect_state.state !=
               ECHOEAR_RUNTIME_RECONNECT_STATE_ERROR;
}

bool echoear_runtime_reconnect_is_offline(void)
{
    return !reconnect_state.network_ready &&
           (reconnect_state.state ==
                ECHOEAR_RUNTIME_RECONNECT_STATE_RETRY_WAIT ||
            reconnect_state.state ==
                ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECT_REQUESTED ||
            reconnect_state.state ==
                ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECTING ||
            reconnect_state.state ==
                ECHOEAR_RUNTIME_RECONNECT_STATE_OFFLINE_WAIT);
}

bool echoear_runtime_reconnect_parse_disconnect_reason(
    const char *value,
    echoear_runtime_disconnect_reason_t *reason)
{
    if (value == NULL || reason == NULL)
        return false;

    if (strcmp(value, "none") == 0 || value[0] == '\0')
        *reason = ECHOEAR_RUNTIME_DISCONNECT_NONE;
    else if (strcmp(value, "link_lost") == 0)
        *reason = ECHOEAR_RUNTIME_DISCONNECT_LINK_LOST;
    else if (strcmp(value, "beacon_timeout") == 0)
        *reason = ECHOEAR_RUNTIME_DISCONNECT_BEACON_TIMEOUT;
    else if (strcmp(value, "router_restart") == 0)
        *reason = ECHOEAR_RUNTIME_DISCONNECT_ROUTER_RESTART;
    else if (strcmp(value, "dhcp_lost") == 0)
        *reason = ECHOEAR_RUNTIME_DISCONNECT_DHCP_LOST;
    else if (strcmp(value, "driver") == 0)
        *reason = ECHOEAR_RUNTIME_DISCONNECT_DRIVER;
    else if (strcmp(value, "manual") == 0)
        *reason = ECHOEAR_RUNTIME_DISCONNECT_MANUAL;
    else if (strcmp(value, "unknown") == 0)
        *reason = ECHOEAR_RUNTIME_DISCONNECT_UNKNOWN;
    else
        return false;

    return true;
}

const char *echoear_runtime_reconnect_state_name(
    echoear_runtime_reconnect_state_t state)
{
    switch (state) {
    case ECHOEAR_RUNTIME_RECONNECT_STATE_IDLE:
        return "idle";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_MONITORING:
        return "monitoring";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_DISCONNECTED:
        return "disconnected";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_RETRY_WAIT:
        return "retry_wait";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECT_REQUESTED:
        return "reconnect_requested";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECTING:
        return "reconnecting";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_OFFLINE_WAIT:
        return "offline_wait";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_RESTORED:
        return "restored";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_SUSPENDED:
        return "suspended";
    case ECHOEAR_RUNTIME_RECONNECT_STATE_ERROR:
    default:
        return "error";
    }
}

const char *echoear_runtime_disconnect_reason_name(
    echoear_runtime_disconnect_reason_t reason)
{
    switch (reason) {
    case ECHOEAR_RUNTIME_DISCONNECT_LINK_LOST:
        return "link_lost";
    case ECHOEAR_RUNTIME_DISCONNECT_BEACON_TIMEOUT:
        return "beacon_timeout";
    case ECHOEAR_RUNTIME_DISCONNECT_ROUTER_RESTART:
        return "router_restart";
    case ECHOEAR_RUNTIME_DISCONNECT_DHCP_LOST:
        return "dhcp_lost";
    case ECHOEAR_RUNTIME_DISCONNECT_DRIVER:
        return "driver";
    case ECHOEAR_RUNTIME_DISCONNECT_MANUAL:
        return "manual";
    case ECHOEAR_RUNTIME_DISCONNECT_UNKNOWN:
        return "unknown";
    case ECHOEAR_RUNTIME_DISCONNECT_NONE:
    default:
        return "none";
    }
}

const char *echoear_runtime_reconnect_error_name(
    echoear_runtime_reconnect_error_t error)
{
    switch (error) {
    case ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE:
        return "none";
    case ECHOEAR_RUNTIME_RECONNECT_ERROR_NOT_MONITORING:
        return "not_monitoring";
    case ECHOEAR_RUNTIME_RECONNECT_ERROR_RECONNECT_FAILED:
        return "reconnect_failed";
    case ECHOEAR_RUNTIME_RECONNECT_ERROR_INTERNAL:
    default:
        return "internal";
    }
}
