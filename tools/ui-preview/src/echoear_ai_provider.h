#ifndef ECHOEAR_AI_PROVIDER_H
#define ECHOEAR_AI_PROVIDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_AI_PROVIDER_MODEL_MAX 64
#define ECHOEAR_AI_PROVIDER_GATEWAY_URL_MAX 160

typedef enum {
    ECHOEAR_AI_PROVIDER_AUTO = 0,
    ECHOEAR_AI_PROVIDER_OPENAI,
    ECHOEAR_AI_PROVIDER_GEMINI,
    ECHOEAR_AI_PROVIDER_DEEPSEEK,
    ECHOEAR_AI_PROVIDER_CLAUDE,
    ECHOEAR_AI_PROVIDER_CUSTOM
} echoear_ai_provider_id_t;

typedef enum {
    ECHOEAR_AI_PROVIDER_STATE_IDLE = 0,
    ECHOEAR_AI_PROVIDER_STATE_UNCONFIGURED,
    ECHOEAR_AI_PROVIDER_STATE_CONFIGURED,
    ECHOEAR_AI_PROVIDER_STATE_CONNECTING,
    ECHOEAR_AI_PROVIDER_STATE_READY,
    ECHOEAR_AI_PROVIDER_STATE_BUSY,
    ECHOEAR_AI_PROVIDER_STATE_DEGRADED,
    ECHOEAR_AI_PROVIDER_STATE_OFFLINE,
    ECHOEAR_AI_PROVIDER_STATE_ERROR
} echoear_ai_provider_state_t;

typedef enum {
    ECHOEAR_AI_PROVIDER_ERROR_NONE = 0,
    ECHOEAR_AI_PROVIDER_ERROR_INVALID_ARGUMENT,
    ECHOEAR_AI_PROVIDER_ERROR_NOT_CONFIGURED,
    ECHOEAR_AI_PROVIDER_ERROR_NETWORK_UNAVAILABLE,
    ECHOEAR_AI_PROVIDER_ERROR_GATEWAY_UNAVAILABLE,
    ECHOEAR_AI_PROVIDER_ERROR_AUTH_FAILED,
    ECHOEAR_AI_PROVIDER_ERROR_PROVIDER_UNAVAILABLE,
    ECHOEAR_AI_PROVIDER_ERROR_RATE_LIMITED,
    ECHOEAR_AI_PROVIDER_ERROR_REQUEST_FAILED,
    ECHOEAR_AI_PROVIDER_ERROR_UNSUPPORTED_CAPABILITY,
    ECHOEAR_AI_PROVIDER_ERROR_INTERNAL
} echoear_ai_provider_error_t;

typedef enum {
    ECHOEAR_AI_CAPABILITY_NONE = 0,
    ECHOEAR_AI_CAPABILITY_CHAT = 1u << 0,
    ECHOEAR_AI_CAPABILITY_TOOLS = 1u << 1,
    ECHOEAR_AI_CAPABILITY_SEARCH = 1u << 2,
    ECHOEAR_AI_CAPABILITY_REALTIME_VOICE = 1u << 3,
    ECHOEAR_AI_CAPABILITY_VISION = 1u << 4
} echoear_ai_capability_t;

typedef struct {
    echoear_ai_provider_id_t primary_provider;
    echoear_ai_provider_id_t fallback_provider;
    bool fallback_enabled;
    bool use_gateway;
    uint32_t required_capabilities;
    char model[ECHOEAR_AI_PROVIDER_MODEL_MAX];
    char gateway_url[ECHOEAR_AI_PROVIDER_GATEWAY_URL_MAX];
} echoear_ai_provider_config_t;

typedef struct {
    echoear_ai_provider_state_t state;
    echoear_ai_provider_error_t error;
    echoear_ai_provider_config_t config;
    echoear_ai_provider_id_t active_provider;
    bool network_ready;
    bool internet_ready;
    bool request_active;
    bool fallback_active;
    uint32_t request_count;
    uint32_t success_count;
    uint32_t failure_count;
    uint32_t fallback_count;
    uint32_t generation;
} echoear_ai_provider_t;

echoear_ai_provider_config_t echoear_ai_provider_default_config(void);
void echoear_ai_provider_init(void);
void echoear_ai_provider_reset(void);
echoear_ai_provider_t *echoear_ai_provider_get(void);

bool echoear_ai_provider_configure(const echoear_ai_provider_config_t *config);
bool echoear_ai_provider_set_primary(echoear_ai_provider_id_t provider);
bool echoear_ai_provider_set_fallback(echoear_ai_provider_id_t provider, bool enabled);
bool echoear_ai_provider_set_model(const char *model);
bool echoear_ai_provider_set_gateway_url(const char *gateway_url);

void echoear_ai_provider_set_connectivity(bool network_ready, bool internet_ready);
void echoear_ai_provider_set_state(echoear_ai_provider_state_t state);
void echoear_ai_provider_set_error(echoear_ai_provider_error_t error);

bool echoear_ai_provider_begin_request(uint32_t required_capabilities);
void echoear_ai_provider_complete_request(void);
void echoear_ai_provider_fail_request(echoear_ai_provider_error_t error);

bool echoear_ai_provider_activate_fallback(void);
void echoear_ai_provider_restore_primary(void);

uint32_t echoear_ai_provider_capabilities(echoear_ai_provider_id_t provider);
bool echoear_ai_provider_supports(echoear_ai_provider_id_t provider, uint32_t required_capabilities);

bool echoear_ai_provider_is_ready(void);
bool echoear_ai_provider_is_busy(void);

const char *echoear_ai_provider_id_to_string(echoear_ai_provider_id_t provider);
echoear_ai_provider_id_t echoear_ai_provider_id_from_string(const char *text);
const char *echoear_ai_provider_state_to_string(echoear_ai_provider_state_t state);
const char *echoear_ai_provider_error_to_string(echoear_ai_provider_error_t error);

#ifdef __cplusplus
}
#endif

#endif
