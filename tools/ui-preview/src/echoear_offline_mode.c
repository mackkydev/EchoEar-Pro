#include "echoear_offline_mode.h"

#include <stddef.h>
#include <string.h>

static echoear_offline_mode_t s_mode;

static uint32_t recovery_delay_for_attempt(uint8_t attempt)
{
    uint32_t delay_ms;
    uint8_t i;

    delay_ms = s_mode.policy.recovery_retry_base_ms;

    if (!s_mode.policy.exponential_backoff || attempt <= 1U) {
        return delay_ms;
    }

    for (i = 1U; i < attempt; i++) {
        if (delay_ms >= s_mode.policy.recovery_retry_max_ms) {
            return s_mode.policy.recovery_retry_max_ms;
        }

        if (delay_ms > s_mode.policy.recovery_retry_max_ms / 2U) {
            return s_mode.policy.recovery_retry_max_ms;
        }

        delay_ms *= 2U;
    }

    if (delay_ms > s_mode.policy.recovery_retry_max_ms) {
        delay_ms = s_mode.policy.recovery_retry_max_ms;
    }

    return delay_ms;
}

static void set_state(
    echoear_offline_mode_state_t state,
    echoear_offline_mode_reason_t reason)
{
    if (s_mode.state != state || s_mode.reason != reason) {
        s_mode.transition_count++;
    }

    s_mode.state = state;
    s_mode.reason = reason;
    s_mode.generation++;
}

static void clear_recovery_runtime(void)
{
    s_mode.recovery_requested = false;
    s_mode.recovery_in_progress = false;
    s_mode.recovery_attempt = 0U;
    s_mode.recovery_retry_delay_ms = 0U;
    s_mode.recovery_retry_due_ms = 0U;
}

static bool had_limited_connectivity(void)
{
    return s_mode.state == ECHOEAR_OFFLINE_MODE_LOCAL_ONLY ||
           s_mode.state == ECHOEAR_OFFLINE_MODE_OFFLINE ||
           s_mode.state == ECHOEAR_OFFLINE_MODE_RECOVERING ||
           s_mode.error == ECHOEAR_OFFLINE_MODE_ERROR_RECOVERY_FAILED;
}

static void request_recovery(uint32_t now_ms)
{
    s_mode.recovery_in_progress = false;
    s_mode.recovery_requested = true;

    if (s_mode.recovery_attempt == 0U) {
        s_mode.recovery_attempt = 1U;
    }

    s_mode.recovery_retry_delay_ms = 0U;
    s_mode.recovery_retry_due_ms = now_ms;

    set_state(
        ECHOEAR_OFFLINE_MODE_RECOVERING,
        ECHOEAR_OFFLINE_MODE_REASON_CONNECTIVITY_RESTORED);
}

echoear_offline_mode_policy_t echoear_offline_mode_default_policy(void)
{
    echoear_offline_mode_policy_t policy;

    policy.recovery_attempts = 3U;
    policy.recovery_retry_base_ms = 2000U;
    policy.recovery_retry_max_ms = 8000U;
    policy.exponential_backoff = true;

    return policy;
}

void echoear_offline_mode_init(void)
{
    memset(&s_mode, 0, sizeof(s_mode));

    s_mode.policy = echoear_offline_mode_default_policy();
    s_mode.state = ECHOEAR_OFFLINE_MODE_IDLE;
    s_mode.reason = ECHOEAR_OFFLINE_MODE_REASON_NONE;
    s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_NONE;
}

void echoear_offline_mode_reset(void)
{
    echoear_offline_mode_init();
}

echoear_offline_mode_t *echoear_offline_mode_get(void)
{
    return &s_mode;
}

void echoear_offline_mode_update(
    const echoear_connectivity_snapshot_t *snapshot,
    uint32_t now_ms)
{
    bool was_limited;

    if (snapshot == NULL) {
        s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_INTERNAL;
        set_state(
            ECHOEAR_OFFLINE_MODE_ERROR,
            ECHOEAR_OFFLINE_MODE_REASON_NONE);
        return;
    }

    if (s_mode.state == ECHOEAR_OFFLINE_MODE_SUSPENDED) {
        s_mode.network_ready = snapshot->network_ready;
        s_mode.local_available = snapshot->local_available;
        s_mode.internet_ready = snapshot->internet_ready;
        s_mode.generation++;
        return;
    }

    was_limited = had_limited_connectivity();

    s_mode.network_ready = snapshot->network_ready;
    s_mode.local_available = snapshot->local_available;
    s_mode.internet_ready = snapshot->internet_ready;

    /*
     * 4C.4 is an availability/policy layer only.
     * It never erases credentials, starts provisioning, or requests
     * Wi-Fi reconnect. Lower network layers remain responsible for that.
     */
    if (!s_mode.network_ready) {
        clear_recovery_runtime();
        s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_NONE;
        set_state(
            ECHOEAR_OFFLINE_MODE_OFFLINE,
            ECHOEAR_OFFLINE_MODE_REASON_NETWORK_UNAVAILABLE);
        return;
    }

    if (!s_mode.internet_ready) {
        clear_recovery_runtime();
        s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_NONE;

        if (s_mode.local_available) {
            set_state(
                ECHOEAR_OFFLINE_MODE_LOCAL_ONLY,
                ECHOEAR_OFFLINE_MODE_REASON_INTERNET_UNAVAILABLE);
        }
        else {
            set_state(
                ECHOEAR_OFFLINE_MODE_OFFLINE,
                ECHOEAR_OFFLINE_MODE_REASON_LOCAL_UNAVAILABLE);
        }
        return;
    }

    if (was_limited) {
        s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_NONE;
        request_recovery(now_ms);
        return;
    }

    clear_recovery_runtime();
    s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_NONE;
    set_state(
        ECHOEAR_OFFLINE_MODE_ONLINE,
        ECHOEAR_OFFLINE_MODE_REASON_INTERNET_READY);
}

void echoear_offline_mode_tick(uint32_t now_ms)
{
    if (s_mode.state != ECHOEAR_OFFLINE_MODE_RECOVERING ||
        s_mode.recovery_in_progress ||
        s_mode.recovery_requested ||
        !s_mode.network_ready ||
        !s_mode.internet_ready) {
        return;
    }

    if (s_mode.recovery_retry_due_ms != 0U &&
        now_ms >= s_mode.recovery_retry_due_ms) {
        s_mode.recovery_retry_delay_ms = 0U;
        s_mode.recovery_retry_due_ms = 0U;
        s_mode.recovery_requested = true;
        s_mode.generation++;
    }
}

bool echoear_offline_mode_take_recovery_request(void)
{
    if (!s_mode.recovery_requested) {
        return false;
    }

    s_mode.recovery_requested = false;
    s_mode.recovery_in_progress = true;
    s_mode.generation++;
    return true;
}

void echoear_offline_mode_mark_recovery_started(void)
{
    if (s_mode.state != ECHOEAR_OFFLINE_MODE_RECOVERING) {
        return;
    }

    s_mode.recovery_requested = false;
    s_mode.recovery_in_progress = true;
    s_mode.generation++;
}

void echoear_offline_mode_mark_recovery_completed(void)
{
    if (!s_mode.network_ready || !s_mode.internet_ready) {
        return;
    }

    clear_recovery_runtime();
    s_mode.recovery_count++;
    s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_NONE;

    set_state(
        ECHOEAR_OFFLINE_MODE_ONLINE,
        ECHOEAR_OFFLINE_MODE_REASON_INTERNET_READY);
}

void echoear_offline_mode_mark_recovery_failed(uint32_t now_ms)
{
    uint32_t delay_ms;

    if (s_mode.state != ECHOEAR_OFFLINE_MODE_RECOVERING) {
        return;
    }

    s_mode.recovery_requested = false;
    s_mode.recovery_in_progress = false;
    s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_RECOVERY_FAILED;

    if (!s_mode.network_ready || !s_mode.internet_ready) {
        return;
    }

    if (s_mode.recovery_attempt >= s_mode.policy.recovery_attempts) {
        s_mode.recovery_retry_delay_ms = 0U;
        s_mode.recovery_retry_due_ms = 0U;
        set_state(
            ECHOEAR_OFFLINE_MODE_RECOVERING,
            ECHOEAR_OFFLINE_MODE_REASON_RECOVERY_FAILED);
        return;
    }

    s_mode.recovery_attempt++;

    delay_ms = recovery_delay_for_attempt(
        s_mode.recovery_attempt - 1U);

    s_mode.recovery_retry_delay_ms = delay_ms;
    s_mode.recovery_retry_due_ms = now_ms + delay_ms;

    set_state(
        ECHOEAR_OFFLINE_MODE_RECOVERING,
        ECHOEAR_OFFLINE_MODE_REASON_RECOVERY_FAILED);
}

void echoear_offline_mode_suspend(void)
{
    s_mode.recovery_requested = false;
    s_mode.recovery_in_progress = false;

    set_state(
        ECHOEAR_OFFLINE_MODE_SUSPENDED,
        ECHOEAR_OFFLINE_MODE_REASON_MANUAL);
}

void echoear_offline_mode_resume(
    const echoear_connectivity_snapshot_t *snapshot,
    uint32_t now_ms)
{
    s_mode.state = ECHOEAR_OFFLINE_MODE_IDLE;
    s_mode.reason = ECHOEAR_OFFLINE_MODE_REASON_NONE;
    echoear_offline_mode_update(snapshot, now_ms);
}

void echoear_offline_mode_clear_error(void)
{
    s_mode.error = ECHOEAR_OFFLINE_MODE_ERROR_NONE;
    s_mode.generation++;
}

bool echoear_offline_mode_is_online(void)
{
    return s_mode.state == ECHOEAR_OFFLINE_MODE_ONLINE;
}

bool echoear_offline_mode_is_local_only(void)
{
    return s_mode.state == ECHOEAR_OFFLINE_MODE_LOCAL_ONLY;
}

bool echoear_offline_mode_is_offline(void)
{
    return s_mode.state == ECHOEAR_OFFLINE_MODE_OFFLINE;
}

bool echoear_offline_mode_is_recovering(void)
{
    return s_mode.state == ECHOEAR_OFFLINE_MODE_RECOVERING;
}

const char *echoear_offline_mode_state_name(
    echoear_offline_mode_state_t state)
{
    switch (state) {
    case ECHOEAR_OFFLINE_MODE_IDLE: return "idle";
    case ECHOEAR_OFFLINE_MODE_ONLINE: return "online";
    case ECHOEAR_OFFLINE_MODE_LOCAL_ONLY: return "local_only";
    case ECHOEAR_OFFLINE_MODE_OFFLINE: return "offline";
    case ECHOEAR_OFFLINE_MODE_RECOVERING: return "recovering";
    case ECHOEAR_OFFLINE_MODE_SUSPENDED: return "suspended";
    case ECHOEAR_OFFLINE_MODE_ERROR: return "error";
    default: return "unknown";
    }
}

const char *echoear_offline_mode_reason_name(
    echoear_offline_mode_reason_t reason)
{
    switch (reason) {
    case ECHOEAR_OFFLINE_MODE_REASON_NONE: return "none";
    case ECHOEAR_OFFLINE_MODE_REASON_INTERNET_READY: return "internet_ready";
    case ECHOEAR_OFFLINE_MODE_REASON_INTERNET_UNAVAILABLE: return "internet_unavailable";
    case ECHOEAR_OFFLINE_MODE_REASON_LOCAL_UNAVAILABLE: return "local_unavailable";
    case ECHOEAR_OFFLINE_MODE_REASON_NETWORK_UNAVAILABLE: return "network_unavailable";
    case ECHOEAR_OFFLINE_MODE_REASON_CONNECTIVITY_RESTORED: return "connectivity_restored";
    case ECHOEAR_OFFLINE_MODE_REASON_RECOVERY_FAILED: return "recovery_failed";
    case ECHOEAR_OFFLINE_MODE_REASON_MANUAL: return "manual";
    default: return "unknown";
    }
}

const char *echoear_offline_mode_error_name(
    echoear_offline_mode_error_t error)
{
    switch (error) {
    case ECHOEAR_OFFLINE_MODE_ERROR_NONE: return "none";
    case ECHOEAR_OFFLINE_MODE_ERROR_RECOVERY_FAILED: return "recovery_failed";
    case ECHOEAR_OFFLINE_MODE_ERROR_INTERNAL: return "internal";
    default: return "unknown";
    }
}
