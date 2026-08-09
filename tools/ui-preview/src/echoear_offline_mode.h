#ifndef ECHOEAR_OFFLINE_MODE_H
#define ECHOEAR_OFFLINE_MODE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECHOEAR_OFFLINE_MODE_IDLE = 0,
    ECHOEAR_OFFLINE_MODE_ONLINE,
    ECHOEAR_OFFLINE_MODE_LOCAL_ONLY,
    ECHOEAR_OFFLINE_MODE_OFFLINE,
    ECHOEAR_OFFLINE_MODE_RECOVERING,
    ECHOEAR_OFFLINE_MODE_SUSPENDED,
    ECHOEAR_OFFLINE_MODE_ERROR
} echoear_offline_mode_state_t;

typedef enum {
    ECHOEAR_OFFLINE_MODE_REASON_NONE = 0,
    ECHOEAR_OFFLINE_MODE_REASON_INTERNET_READY,
    ECHOEAR_OFFLINE_MODE_REASON_INTERNET_UNAVAILABLE,
    ECHOEAR_OFFLINE_MODE_REASON_LOCAL_UNAVAILABLE,
    ECHOEAR_OFFLINE_MODE_REASON_NETWORK_UNAVAILABLE,
    ECHOEAR_OFFLINE_MODE_REASON_CONNECTIVITY_RESTORED,
    ECHOEAR_OFFLINE_MODE_REASON_RECOVERY_FAILED,
    ECHOEAR_OFFLINE_MODE_REASON_MANUAL
} echoear_offline_mode_reason_t;

typedef enum {
    ECHOEAR_OFFLINE_MODE_ERROR_NONE = 0,
    ECHOEAR_OFFLINE_MODE_ERROR_RECOVERY_FAILED,
    ECHOEAR_OFFLINE_MODE_ERROR_INTERNAL
} echoear_offline_mode_error_t;

typedef struct {
    uint8_t recovery_attempts;
    uint32_t recovery_retry_base_ms;
    uint32_t recovery_retry_max_ms;
    bool exponential_backoff;
} echoear_offline_mode_policy_t;

typedef struct {
    bool network_ready;
    bool local_available;
    bool internet_ready;
} echoear_connectivity_snapshot_t;

typedef struct {
    echoear_offline_mode_state_t state;
    echoear_offline_mode_reason_t reason;
    echoear_offline_mode_error_t error;
    echoear_offline_mode_policy_t policy;

    bool network_ready;
    bool local_available;
    bool internet_ready;

    bool recovery_requested;
    bool recovery_in_progress;

    uint8_t recovery_attempt;
    uint32_t recovery_retry_delay_ms;
    uint32_t recovery_retry_due_ms;

    uint32_t transition_count;
    uint32_t recovery_count;
    uint32_t generation;
} echoear_offline_mode_t;

void echoear_offline_mode_init(void);
void echoear_offline_mode_reset(void);
echoear_offline_mode_t *echoear_offline_mode_get(void);
echoear_offline_mode_policy_t echoear_offline_mode_default_policy(void);

void echoear_offline_mode_update(
    const echoear_connectivity_snapshot_t *snapshot,
    uint32_t now_ms);

void echoear_offline_mode_tick(uint32_t now_ms);

bool echoear_offline_mode_take_recovery_request(void);
void echoear_offline_mode_mark_recovery_started(void);
void echoear_offline_mode_mark_recovery_completed(void);
void echoear_offline_mode_mark_recovery_failed(uint32_t now_ms);

void echoear_offline_mode_suspend(void);
void echoear_offline_mode_resume(
    const echoear_connectivity_snapshot_t *snapshot,
    uint32_t now_ms);

void echoear_offline_mode_clear_error(void);

bool echoear_offline_mode_is_online(void);
bool echoear_offline_mode_is_local_only(void);
bool echoear_offline_mode_is_offline(void);
bool echoear_offline_mode_is_recovering(void);

const char *echoear_offline_mode_state_name(
    echoear_offline_mode_state_t state);
const char *echoear_offline_mode_reason_name(
    echoear_offline_mode_reason_t reason);
const char *echoear_offline_mode_error_name(
    echoear_offline_mode_error_t error);

#ifdef __cplusplus
}
#endif

#endif
