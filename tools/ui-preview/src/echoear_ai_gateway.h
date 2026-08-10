#ifndef ECHOEAR_AI_GATEWAY_H
#define ECHOEAR_AI_GATEWAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_AI_GATEWAY_URL_MAX        192
#define ECHOEAR_AI_GATEWAY_DEVICE_ID_MAX   64
#define ECHOEAR_AI_GATEWAY_MAX_RETRIES      3

typedef enum {
    ECHOEAR_AI_GATEWAY_STATE_IDLE = 0,
    ECHOEAR_AI_GATEWAY_STATE_UNCONFIGURED,
    ECHOEAR_AI_GATEWAY_STATE_CONFIGURED,
    ECHOEAR_AI_GATEWAY_STATE_OFFLINE,
    ECHOEAR_AI_GATEWAY_STATE_CONNECT_REQUESTED,
    ECHOEAR_AI_GATEWAY_STATE_CONNECTING,
    ECHOEAR_AI_GATEWAY_STATE_AUTHENTICATING,
    ECHOEAR_AI_GATEWAY_STATE_READY,
    ECHOEAR_AI_GATEWAY_STATE_DEGRADED,
    ECHOEAR_AI_GATEWAY_STATE_BACKOFF,
    ECHOEAR_AI_GATEWAY_STATE_ERROR
} echoear_ai_gateway_state_t;

typedef enum {
    ECHOEAR_AI_GATEWAY_ERROR_NONE = 0,
    ECHOEAR_AI_GATEWAY_ERROR_INVALID_ARGUMENT,
    ECHOEAR_AI_GATEWAY_ERROR_NOT_CONFIGURED,
    ECHOEAR_AI_GATEWAY_ERROR_NETWORK_UNAVAILABLE,
    ECHOEAR_AI_GATEWAY_ERROR_INTERNET_UNAVAILABLE,
    ECHOEAR_AI_GATEWAY_ERROR_DEVICE_ID_MISSING,
    ECHOEAR_AI_GATEWAY_ERROR_CREDENTIAL_MISSING,
    ECHOEAR_AI_GATEWAY_ERROR_DNS_FAILED,
    ECHOEAR_AI_GATEWAY_ERROR_CONNECT_FAILED,
    ECHOEAR_AI_GATEWAY_ERROR_TLS_FAILED,
    ECHOEAR_AI_GATEWAY_ERROR_AUTH_FAILED,
    ECHOEAR_AI_GATEWAY_ERROR_SESSION_EXPIRED,
    ECHOEAR_AI_GATEWAY_ERROR_RATE_LIMITED,
    ECHOEAR_AI_GATEWAY_ERROR_SERVER_UNAVAILABLE,
    ECHOEAR_AI_GATEWAY_ERROR_PROTOCOL,
    ECHOEAR_AI_GATEWAY_ERROR_INTERNAL
} echoear_ai_gateway_error_t;

typedef struct {
    bool enabled;
    bool auto_connect;
    uint32_t max_retries;
    uint32_t retry_base_delay_ms;
    uint32_t retry_max_delay_ms;
    char gateway_url[ECHOEAR_AI_GATEWAY_URL_MAX];
} echoear_ai_gateway_config_t;

typedef struct {
    echoear_ai_gateway_state_t state;
    echoear_ai_gateway_error_t error;
    echoear_ai_gateway_config_t config;
    char device_id[ECHOEAR_AI_GATEWAY_DEVICE_ID_MAX];
    bool credential_ready;
    bool network_ready;
    bool internet_ready;
    bool transport_ready;
    bool authenticated;
    bool session_ready;
    bool connect_request;
    uint32_t retry_attempt;
    uint32_t retry_delay_ms;
    uint32_t retry_due_ms;
    uint32_t now_ms;
    uint32_t connect_count;
    uint32_t auth_count;
    uint32_t session_count;
    uint32_t failure_count;
    uint32_t generation;
} echoear_ai_gateway_t;

echoear_ai_gateway_config_t echoear_ai_gateway_default_config(void);
void echoear_ai_gateway_init(void);
void echoear_ai_gateway_reset(void);
echoear_ai_gateway_t *echoear_ai_gateway_get(void);
bool echoear_ai_gateway_configure(const echoear_ai_gateway_config_t *config);
bool echoear_ai_gateway_set_url(const char *gateway_url);
bool echoear_ai_gateway_set_device_id(const char *device_id);
void echoear_ai_gateway_set_credential_ready(bool ready);
void echoear_ai_gateway_set_connectivity(bool network_ready, bool internet_ready);
bool echoear_ai_gateway_request_connect(void);
bool echoear_ai_gateway_begin_connect(void);
void echoear_ai_gateway_transport_connected(void);
void echoear_ai_gateway_auth_succeeded(void);
void echoear_ai_gateway_fail(echoear_ai_gateway_error_t error);
void echoear_ai_gateway_disconnect(echoear_ai_gateway_error_t reason);
void echoear_ai_gateway_session_expired(void);
void echoear_ai_gateway_tick(uint32_t now_ms);
bool echoear_ai_gateway_is_configured(void);
bool echoear_ai_gateway_is_ready(void);
bool echoear_ai_gateway_should_connect(void);
const char *echoear_ai_gateway_state_name(echoear_ai_gateway_state_t state);
const char *echoear_ai_gateway_error_name(echoear_ai_gateway_error_t error);

#ifdef __cplusplus
}
#endif

#endif
