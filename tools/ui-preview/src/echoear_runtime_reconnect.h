#ifndef ECHOEAR_RUNTIME_RECONNECT_H
#define ECHOEAR_RUNTIME_RECONNECT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ECHOEAR_RUNTIME_RECONNECT_STATE_IDLE = 0,
    ECHOEAR_RUNTIME_RECONNECT_STATE_MONITORING,
    ECHOEAR_RUNTIME_RECONNECT_STATE_DISCONNECTED,
    ECHOEAR_RUNTIME_RECONNECT_STATE_RETRY_WAIT,
    ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECT_REQUESTED,
    ECHOEAR_RUNTIME_RECONNECT_STATE_RECONNECTING,
    ECHOEAR_RUNTIME_RECONNECT_STATE_OFFLINE_WAIT,
    ECHOEAR_RUNTIME_RECONNECT_STATE_RESTORED,
    ECHOEAR_RUNTIME_RECONNECT_STATE_SUSPENDED,
    ECHOEAR_RUNTIME_RECONNECT_STATE_ERROR
} echoear_runtime_reconnect_state_t;

typedef enum {
    ECHOEAR_RUNTIME_DISCONNECT_NONE = 0,
    ECHOEAR_RUNTIME_DISCONNECT_LINK_LOST,
    ECHOEAR_RUNTIME_DISCONNECT_BEACON_TIMEOUT,
    ECHOEAR_RUNTIME_DISCONNECT_ROUTER_RESTART,
    ECHOEAR_RUNTIME_DISCONNECT_DHCP_LOST,
    ECHOEAR_RUNTIME_DISCONNECT_DRIVER,
    ECHOEAR_RUNTIME_DISCONNECT_MANUAL,
    ECHOEAR_RUNTIME_DISCONNECT_UNKNOWN
} echoear_runtime_disconnect_reason_t;

typedef enum {
    ECHOEAR_RUNTIME_RECONNECT_ERROR_NONE = 0,
    ECHOEAR_RUNTIME_RECONNECT_ERROR_NOT_MONITORING,
    ECHOEAR_RUNTIME_RECONNECT_ERROR_RECONNECT_FAILED,
    ECHOEAR_RUNTIME_RECONNECT_ERROR_INTERNAL
} echoear_runtime_reconnect_error_t;

typedef struct {
    uint8_t fast_attempts;
    uint32_t fast_retry_base_ms;
    uint32_t fast_retry_max_ms;
    uint32_t offline_retry_interval_ms;
    bool exponential_backoff;
} echoear_runtime_reconnect_policy_t;

typedef struct {
    echoear_runtime_reconnect_state_t state;
    echoear_runtime_disconnect_reason_t disconnect_reason;
    echoear_runtime_reconnect_error_t error;
    echoear_runtime_reconnect_policy_t policy;

    bool network_ready;
    bool reconnect_requested;
    bool user_disconnect;
    bool provisioning_allowed;

    uint8_t fast_attempt;
    uint32_t retry_delay_ms;
    uint32_t retry_due_ms;

    uint32_t disconnect_count;
    uint32_t reconnect_count;
    uint32_t generation;
} echoear_runtime_reconnect_t;

void echoear_runtime_reconnect_init(void);
void echoear_runtime_reconnect_reset(void);
echoear_runtime_reconnect_t *echoear_runtime_reconnect_get(void);

echoear_runtime_reconnect_policy_t
echoear_runtime_reconnect_default_policy(void);

void echoear_runtime_reconnect_mark_network_ready(void);

bool echoear_runtime_reconnect_report_disconnect(
    echoear_runtime_disconnect_reason_t reason,
    uint32_t now_ms);

void echoear_runtime_reconnect_tick(uint32_t now_ms);

bool echoear_runtime_reconnect_take_request(void);
void echoear_runtime_reconnect_mark_connecting(void);
void echoear_runtime_reconnect_mark_restored(void);
void echoear_runtime_reconnect_mark_failed(uint32_t now_ms);

void echoear_runtime_reconnect_suspend(void);
void echoear_runtime_reconnect_resume(
    bool network_ready,
    uint32_t now_ms);

void echoear_runtime_reconnect_clear_error(void);

bool echoear_runtime_reconnect_is_active(void);
bool echoear_runtime_reconnect_is_offline(void);

bool echoear_runtime_reconnect_parse_disconnect_reason(
    const char *value,
    echoear_runtime_disconnect_reason_t *reason);

const char *echoear_runtime_reconnect_state_name(
    echoear_runtime_reconnect_state_t state);

const char *echoear_runtime_disconnect_reason_name(
    echoear_runtime_disconnect_reason_t reason);

const char *echoear_runtime_reconnect_error_name(
    echoear_runtime_reconnect_error_t error);

#endif
