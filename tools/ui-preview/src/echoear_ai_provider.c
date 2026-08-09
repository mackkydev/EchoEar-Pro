#include "echoear_ai_provider.h"

#include <string.h>

static echoear_ai_provider_t s_ai_provider;

static bool provider_is_valid(echoear_ai_provider_id_t provider)
{
    return provider >= ECHOEAR_AI_PROVIDER_AUTO &&
           provider <= ECHOEAR_AI_PROVIDER_CUSTOM;
}

static void copy_text(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0) return;
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    size_t len = strlen(src);
    if (len >= dst_size) len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static bool config_is_complete(const echoear_ai_provider_config_t *config)
{
    if (config == NULL) return false;
    if (!provider_is_valid(config->primary_provider)) return false;
    if (!provider_is_valid(config->fallback_provider)) return false;
    if (config->use_gateway && config->gateway_url[0] == '\0') return false;
    return true;
}

static void refresh_configuration_state(void)
{
    if (!config_is_complete(&s_ai_provider.config)) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_UNCONFIGURED;
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NOT_CONFIGURED;
        s_ai_provider.generation++;
        return;
    }

    if (!s_ai_provider.network_ready || !s_ai_provider.internet_ready) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_OFFLINE;
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NETWORK_UNAVAILABLE;
        s_ai_provider.generation++;
        return;
    }

    if (!s_ai_provider.request_active) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_CONFIGURED;
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NONE;
        s_ai_provider.generation++;
    }
}

echoear_ai_provider_config_t echoear_ai_provider_default_config(void)
{
    echoear_ai_provider_config_t config;
    memset(&config, 0, sizeof(config));

    config.primary_provider = ECHOEAR_AI_PROVIDER_OPENAI;
    config.fallback_provider = ECHOEAR_AI_PROVIDER_GEMINI;
    config.fallback_enabled = true;
    config.use_gateway = true;
    config.required_capabilities =
        ECHOEAR_AI_CAPABILITY_CHAT |
        ECHOEAR_AI_CAPABILITY_TOOLS;

    return config;
}

void echoear_ai_provider_init(void)
{
    memset(&s_ai_provider, 0, sizeof(s_ai_provider));
    s_ai_provider.config = echoear_ai_provider_default_config();
    s_ai_provider.active_provider = s_ai_provider.config.primary_provider;
    s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_UNCONFIGURED;
    s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NOT_CONFIGURED;
    s_ai_provider.generation = 1;
}

void echoear_ai_provider_reset(void)
{
    echoear_ai_provider_init();
}

echoear_ai_provider_t *echoear_ai_provider_get(void)
{
    return &s_ai_provider;
}

bool echoear_ai_provider_configure(const echoear_ai_provider_config_t *config)
{
    if (!config_is_complete(config)) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_UNCONFIGURED;
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NOT_CONFIGURED;
        s_ai_provider.generation++;
        return false;
    }

    s_ai_provider.config = *config;
    s_ai_provider.active_provider = config->primary_provider;
    s_ai_provider.fallback_active = false;
    s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NONE;
    refresh_configuration_state();
    return true;
}

bool echoear_ai_provider_set_primary(echoear_ai_provider_id_t provider)
{
    if (!provider_is_valid(provider)) {
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_INVALID_ARGUMENT;
        s_ai_provider.generation++;
        return false;
    }

    s_ai_provider.config.primary_provider = provider;
    if (!s_ai_provider.fallback_active) {
        s_ai_provider.active_provider = provider;
    }
    s_ai_provider.generation++;
    return true;
}

bool echoear_ai_provider_set_fallback(echoear_ai_provider_id_t provider, bool enabled)
{
    if (!provider_is_valid(provider)) {
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_INVALID_ARGUMENT;
        s_ai_provider.generation++;
        return false;
    }

    s_ai_provider.config.fallback_provider = provider;
    s_ai_provider.config.fallback_enabled = enabled;
    s_ai_provider.generation++;
    return true;
}

bool echoear_ai_provider_set_model(const char *model)
{
    if (model == NULL) {
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_INVALID_ARGUMENT;
        s_ai_provider.generation++;
        return false;
    }

    copy_text(s_ai_provider.config.model, sizeof(s_ai_provider.config.model), model);
    s_ai_provider.generation++;
    return true;
}

bool echoear_ai_provider_set_gateway_url(const char *gateway_url)
{
    if (gateway_url == NULL) {
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_INVALID_ARGUMENT;
        s_ai_provider.generation++;
        return false;
    }

    copy_text(
        s_ai_provider.config.gateway_url,
        sizeof(s_ai_provider.config.gateway_url),
        gateway_url);

    refresh_configuration_state();
    return true;
}

void echoear_ai_provider_set_connectivity(bool network_ready, bool internet_ready)
{
    bool changed =
        s_ai_provider.network_ready != network_ready ||
        s_ai_provider.internet_ready != internet_ready;

    s_ai_provider.network_ready = network_ready;
    s_ai_provider.internet_ready = internet_ready;

    if (!changed) return;

    if (!network_ready || !internet_ready) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_OFFLINE;
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NETWORK_UNAVAILABLE;
        s_ai_provider.request_active = false;
        s_ai_provider.generation++;
        return;
    }

    refresh_configuration_state();
}

void echoear_ai_provider_set_state(echoear_ai_provider_state_t state)
{
    s_ai_provider.state = state;
    s_ai_provider.generation++;
}

void echoear_ai_provider_set_error(echoear_ai_provider_error_t error)
{
    s_ai_provider.error = error;
    s_ai_provider.generation++;
}

bool echoear_ai_provider_begin_request(uint32_t required_capabilities)
{
    if (!config_is_complete(&s_ai_provider.config)) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_UNCONFIGURED;
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NOT_CONFIGURED;
        s_ai_provider.generation++;
        return false;
    }

    if (!s_ai_provider.network_ready || !s_ai_provider.internet_ready) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_OFFLINE;
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NETWORK_UNAVAILABLE;
        s_ai_provider.generation++;
        return false;
    }

    if (s_ai_provider.request_active) {
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_REQUEST_FAILED;
        s_ai_provider.generation++;
        return false;
    }

    if (!echoear_ai_provider_supports(
            s_ai_provider.active_provider,
            required_capabilities)) {
        s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_UNSUPPORTED_CAPABILITY;
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_DEGRADED;
        s_ai_provider.generation++;
        return false;
    }

    s_ai_provider.request_active = true;
    s_ai_provider.request_count++;
    s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_BUSY;
    s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NONE;
    s_ai_provider.generation++;
    return true;
}

void echoear_ai_provider_complete_request(void)
{
    if (!s_ai_provider.request_active) return;

    s_ai_provider.request_active = false;
    s_ai_provider.success_count++;
    s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_READY;
    s_ai_provider.error = ECHOEAR_AI_PROVIDER_ERROR_NONE;
    s_ai_provider.generation++;
}

void echoear_ai_provider_fail_request(echoear_ai_provider_error_t error)
{
    if (s_ai_provider.request_active) {
        s_ai_provider.request_active = false;
        s_ai_provider.failure_count++;
    }

    s_ai_provider.error = error;

    if (error == ECHOEAR_AI_PROVIDER_ERROR_NETWORK_UNAVAILABLE ||
        error == ECHOEAR_AI_PROVIDER_ERROR_GATEWAY_UNAVAILABLE) {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_OFFLINE;
    } else {
        s_ai_provider.state = ECHOEAR_AI_PROVIDER_STATE_DEGRADED;
    }

    s_ai_provider.generation++;
}

bool echoear_ai_provider_activate_fallback(void)
{
    if (!s_ai_provider.config.fallback_enabled) return false;

    if (!provider_is_valid(s_ai_provider.config.fallback_provider) ||
        s_ai_provider.config.fallback_provider ==
            s_ai_provider.config.primary_provider) {
        return false;
    }

    s_ai_provider.active_provider = s_ai_provider.config.fallback_provider;
    s_ai_provider.fallback_active = true;
    s_ai_provider.fallback_count++;
    s_ai_provider.generation++;
    return true;
}

void echoear_ai_provider_restore_primary(void)
{
    s_ai_provider.active_provider = s_ai_provider.config.primary_provider;
    s_ai_provider.fallback_active = false;
    s_ai_provider.generation++;
}

uint32_t echoear_ai_provider_capabilities(echoear_ai_provider_id_t provider)
{
    switch (provider) {
        case ECHOEAR_AI_PROVIDER_OPENAI:
            return ECHOEAR_AI_CAPABILITY_CHAT |
                   ECHOEAR_AI_CAPABILITY_TOOLS |
                   ECHOEAR_AI_CAPABILITY_SEARCH |
                   ECHOEAR_AI_CAPABILITY_REALTIME_VOICE |
                   ECHOEAR_AI_CAPABILITY_VISION;

        case ECHOEAR_AI_PROVIDER_GEMINI:
            return ECHOEAR_AI_CAPABILITY_CHAT |
                   ECHOEAR_AI_CAPABILITY_TOOLS |
                   ECHOEAR_AI_CAPABILITY_SEARCH |
                   ECHOEAR_AI_CAPABILITY_REALTIME_VOICE |
                   ECHOEAR_AI_CAPABILITY_VISION;

        case ECHOEAR_AI_PROVIDER_DEEPSEEK:
            return ECHOEAR_AI_CAPABILITY_CHAT |
                   ECHOEAR_AI_CAPABILITY_TOOLS;

        case ECHOEAR_AI_PROVIDER_CLAUDE:
            return ECHOEAR_AI_CAPABILITY_CHAT |
                   ECHOEAR_AI_CAPABILITY_TOOLS |
                   ECHOEAR_AI_CAPABILITY_SEARCH |
                   ECHOEAR_AI_CAPABILITY_VISION;

        case ECHOEAR_AI_PROVIDER_CUSTOM:
            return ECHOEAR_AI_CAPABILITY_CHAT |
                   ECHOEAR_AI_CAPABILITY_TOOLS;

        case ECHOEAR_AI_PROVIDER_AUTO:
        default:
            return ECHOEAR_AI_CAPABILITY_CHAT |
                   ECHOEAR_AI_CAPABILITY_TOOLS |
                   ECHOEAR_AI_CAPABILITY_SEARCH |
                   ECHOEAR_AI_CAPABILITY_REALTIME_VOICE |
                   ECHOEAR_AI_CAPABILITY_VISION;
    }
}

bool echoear_ai_provider_supports(
    echoear_ai_provider_id_t provider,
    uint32_t required_capabilities)
{
    uint32_t available = echoear_ai_provider_capabilities(provider);
    return (available & required_capabilities) == required_capabilities;
}

bool echoear_ai_provider_is_ready(void)
{
    return s_ai_provider.state == ECHOEAR_AI_PROVIDER_STATE_READY ||
           s_ai_provider.state == ECHOEAR_AI_PROVIDER_STATE_CONFIGURED;
}

bool echoear_ai_provider_is_busy(void)
{
    return s_ai_provider.request_active ||
           s_ai_provider.state == ECHOEAR_AI_PROVIDER_STATE_BUSY;
}

const char *echoear_ai_provider_id_to_string(echoear_ai_provider_id_t provider)
{
    switch (provider) {
        case ECHOEAR_AI_PROVIDER_AUTO: return "auto";
        case ECHOEAR_AI_PROVIDER_OPENAI: return "openai";
        case ECHOEAR_AI_PROVIDER_GEMINI: return "gemini";
        case ECHOEAR_AI_PROVIDER_DEEPSEEK: return "deepseek";
        case ECHOEAR_AI_PROVIDER_CLAUDE: return "claude";
        case ECHOEAR_AI_PROVIDER_CUSTOM: return "custom";
        default: return "unknown";
    }
}

echoear_ai_provider_id_t echoear_ai_provider_id_from_string(const char *text)
{
    if (text == NULL) return ECHOEAR_AI_PROVIDER_AUTO;
    if (strcmp(text, "openai") == 0) return ECHOEAR_AI_PROVIDER_OPENAI;
    if (strcmp(text, "gemini") == 0) return ECHOEAR_AI_PROVIDER_GEMINI;
    if (strcmp(text, "deepseek") == 0) return ECHOEAR_AI_PROVIDER_DEEPSEEK;
    if (strcmp(text, "claude") == 0 || strcmp(text, "anthropic") == 0)
        return ECHOEAR_AI_PROVIDER_CLAUDE;
    if (strcmp(text, "custom") == 0) return ECHOEAR_AI_PROVIDER_CUSTOM;
    return ECHOEAR_AI_PROVIDER_AUTO;
}

const char *echoear_ai_provider_state_to_string(echoear_ai_provider_state_t state)
{
    switch (state) {
        case ECHOEAR_AI_PROVIDER_STATE_IDLE: return "idle";
        case ECHOEAR_AI_PROVIDER_STATE_UNCONFIGURED: return "unconfigured";
        case ECHOEAR_AI_PROVIDER_STATE_CONFIGURED: return "configured";
        case ECHOEAR_AI_PROVIDER_STATE_CONNECTING: return "connecting";
        case ECHOEAR_AI_PROVIDER_STATE_READY: return "ready";
        case ECHOEAR_AI_PROVIDER_STATE_BUSY: return "busy";
        case ECHOEAR_AI_PROVIDER_STATE_DEGRADED: return "degraded";
        case ECHOEAR_AI_PROVIDER_STATE_OFFLINE: return "offline";
        case ECHOEAR_AI_PROVIDER_STATE_ERROR: return "error";
        default: return "unknown";
    }
}

const char *echoear_ai_provider_error_to_string(echoear_ai_provider_error_t error)
{
    switch (error) {
        case ECHOEAR_AI_PROVIDER_ERROR_NONE: return "none";
        case ECHOEAR_AI_PROVIDER_ERROR_INVALID_ARGUMENT: return "invalid_argument";
        case ECHOEAR_AI_PROVIDER_ERROR_NOT_CONFIGURED: return "not_configured";
        case ECHOEAR_AI_PROVIDER_ERROR_NETWORK_UNAVAILABLE: return "network_unavailable";
        case ECHOEAR_AI_PROVIDER_ERROR_GATEWAY_UNAVAILABLE: return "gateway_unavailable";
        case ECHOEAR_AI_PROVIDER_ERROR_AUTH_FAILED: return "auth_failed";
        case ECHOEAR_AI_PROVIDER_ERROR_PROVIDER_UNAVAILABLE: return "provider_unavailable";
        case ECHOEAR_AI_PROVIDER_ERROR_RATE_LIMITED: return "rate_limited";
        case ECHOEAR_AI_PROVIDER_ERROR_REQUEST_FAILED: return "request_failed";
        case ECHOEAR_AI_PROVIDER_ERROR_UNSUPPORTED_CAPABILITY: return "unsupported_capability";
        case ECHOEAR_AI_PROVIDER_ERROR_INTERNAL: return "internal";
        default: return "unknown";
    }
}
