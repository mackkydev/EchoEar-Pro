#ifndef ECHOEAR_NETWORK_HEALTH_H
#define ECHOEAR_NETWORK_HEALTH_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
 ECHOEAR_NETWORK_HEALTH_IDLE=0, ECHOEAR_NETWORK_HEALTH_CHECK_REQUESTED, ECHOEAR_NETWORK_HEALTH_CHECKING,
 ECHOEAR_NETWORK_HEALTH_INTERNET_READY, ECHOEAR_NETWORK_HEALTH_LOCAL_ONLY, ECHOEAR_NETWORK_HEALTH_DEGRADED,
 ECHOEAR_NETWORK_HEALTH_OFFLINE, ECHOEAR_NETWORK_HEALTH_SUSPENDED, ECHOEAR_NETWORK_HEALTH_ERROR
} echoear_network_health_state_t;
typedef enum {
 ECHOEAR_NETWORK_HEALTH_ERROR_NONE=0, ECHOEAR_NETWORK_HEALTH_ERROR_NOT_READY,
 ECHOEAR_NETWORK_HEALTH_ERROR_INVALID_REPORT, ECHOEAR_NETWORK_HEALTH_ERROR_INTERNAL
} echoear_network_health_error_t;
typedef struct { uint32_t healthy_check_interval_ms, degraded_check_interval_ms, local_only_check_interval_ms; uint8_t failure_confirmations; } echoear_network_health_policy_t;
typedef struct { bool gateway_reachable, dns_working, internet_reachable; uint32_t latency_ms; } echoear_network_health_probe_t;
typedef struct {
 echoear_network_health_state_t state; echoear_network_health_error_t error; echoear_network_health_policy_t policy;
 bool network_ready, check_requested, gateway_reachable, dns_working, internet_reachable;
 uint8_t consecutive_failures, consecutive_successes;
 uint32_t check_count, last_check_ms, next_check_ms, last_latency_ms, generation;
} echoear_network_health_t;
void echoear_network_health_init(void);
void echoear_network_health_reset(void);
echoear_network_health_t *echoear_network_health_get(void);
echoear_network_health_policy_t echoear_network_health_default_policy(void);
void echoear_network_health_set_network_ready(bool ready,uint32_t now_ms);
void echoear_network_health_tick(uint32_t now_ms);
bool echoear_network_health_take_check_request(void);
void echoear_network_health_report_probe(const echoear_network_health_probe_t *probe,uint32_t now_ms);
void echoear_network_health_suspend(void);
void echoear_network_health_resume(bool network_ready,uint32_t now_ms);
void echoear_network_health_clear_error(void);
bool echoear_network_health_is_internet_ready(void);
bool echoear_network_health_is_local_available(void);
bool echoear_network_health_is_degraded(void);
bool echoear_network_health_is_offline(void);
const char *echoear_network_health_state_name(echoear_network_health_state_t state);
const char *echoear_network_health_error_name(echoear_network_health_error_t error);
#ifdef __cplusplus
}
#endif
#endif
