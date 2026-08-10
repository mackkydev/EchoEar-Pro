#include "echoear_ai_gateway.h"
#include <string.h>

static echoear_ai_gateway_t s_gateway;

static bool str_nonempty(const char *s) { return s != NULL && s[0] != '\0'; }
static void copy_string(char *dst, size_t n, const char *src) {
    if (!dst || n == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t len = strlen(src); if (len >= n) len = n - 1;
    memcpy(dst, src, len); dst[len] = '\0';
}
static void bump(void) { s_gateway.generation++; }
static void set_state(echoear_ai_gateway_state_t state, echoear_ai_gateway_error_t error) {
    if (s_gateway.state != state || s_gateway.error != error) { s_gateway.state = state; s_gateway.error = error; bump(); }
}
static bool runtime_ready(void) { return str_nonempty(s_gateway.device_id) && s_gateway.credential_ready; }
static bool online(void) { return s_gateway.network_ready && s_gateway.internet_ready; }
static void clear_session(void) {
    s_gateway.connect_request = false; s_gateway.transport_ready = false; s_gateway.authenticated = false; s_gateway.session_ready = false;
}
static uint32_t retry_delay(uint32_t attempt) {
    uint64_t d = s_gateway.config.retry_base_delay_ms ? s_gateway.config.retry_base_delay_ms : 1000;
    while (attempt > 1 && d < s_gateway.config.retry_max_delay_ms) { d *= 2; attempt--; }
    if (s_gateway.config.retry_max_delay_ms && d > s_gateway.config.retry_max_delay_ms) d = s_gateway.config.retry_max_delay_ms;
    return (uint32_t)d;
}
static bool retryable(echoear_ai_gateway_error_t error) {
    switch (error) {
        case ECHOEAR_AI_GATEWAY_ERROR_DNS_FAILED:
        case ECHOEAR_AI_GATEWAY_ERROR_CONNECT_FAILED:
        case ECHOEAR_AI_GATEWAY_ERROR_TLS_FAILED:
        case ECHOEAR_AI_GATEWAY_ERROR_SESSION_EXPIRED:
        case ECHOEAR_AI_GATEWAY_ERROR_RATE_LIMITED:
        case ECHOEAR_AI_GATEWAY_ERROR_SERVER_UNAVAILABLE:
        case ECHOEAR_AI_GATEWAY_ERROR_PROTOCOL: return true;
        default: return false;
    }
}
static void evaluate(void) {
    if (!s_gateway.config.enabled) { clear_session(); set_state(ECHOEAR_AI_GATEWAY_STATE_IDLE, ECHOEAR_AI_GATEWAY_ERROR_NONE); return; }
    if (!echoear_ai_gateway_is_configured()) { clear_session(); set_state(ECHOEAR_AI_GATEWAY_STATE_UNCONFIGURED, ECHOEAR_AI_GATEWAY_ERROR_NOT_CONFIGURED); return; }
    if (!s_gateway.network_ready) { clear_session(); set_state(ECHOEAR_AI_GATEWAY_STATE_OFFLINE, ECHOEAR_AI_GATEWAY_ERROR_NETWORK_UNAVAILABLE); return; }
    if (!s_gateway.internet_ready) { clear_session(); set_state(ECHOEAR_AI_GATEWAY_STATE_OFFLINE, ECHOEAR_AI_GATEWAY_ERROR_INTERNET_UNAVAILABLE); return; }
    if (!str_nonempty(s_gateway.device_id)) { clear_session(); set_state(ECHOEAR_AI_GATEWAY_STATE_CONFIGURED, ECHOEAR_AI_GATEWAY_ERROR_DEVICE_ID_MISSING); return; }
    if (!s_gateway.credential_ready) { clear_session(); set_state(ECHOEAR_AI_GATEWAY_STATE_CONFIGURED, ECHOEAR_AI_GATEWAY_ERROR_CREDENTIAL_MISSING); return; }
    if (!s_gateway.session_ready && s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_CONNECT_REQUESTED && s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_CONNECTING && s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_AUTHENTICATING && s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_BACKOFF)
        set_state(ECHOEAR_AI_GATEWAY_STATE_CONFIGURED, ECHOEAR_AI_GATEWAY_ERROR_NONE);
}

echoear_ai_gateway_config_t echoear_ai_gateway_default_config(void) {
    echoear_ai_gateway_config_t c; memset(&c, 0, sizeof(c)); c.enabled = true; c.auto_connect = true; c.max_retries = 3; c.retry_base_delay_ms = 1000; c.retry_max_delay_ms = 10000; return c;
}
void echoear_ai_gateway_init(void) { memset(&s_gateway, 0, sizeof(s_gateway)); s_gateway.config = echoear_ai_gateway_default_config(); s_gateway.state = ECHOEAR_AI_GATEWAY_STATE_UNCONFIGURED; s_gateway.error = ECHOEAR_AI_GATEWAY_ERROR_NOT_CONFIGURED; s_gateway.generation = 1; }
void echoear_ai_gateway_reset(void) { echoear_ai_gateway_init(); }
echoear_ai_gateway_t *echoear_ai_gateway_get(void) { return &s_gateway; }

bool echoear_ai_gateway_configure(const echoear_ai_gateway_config_t *config) {
    if (!config) { set_state(ECHOEAR_AI_GATEWAY_STATE_ERROR, ECHOEAR_AI_GATEWAY_ERROR_INVALID_ARGUMENT); return false; }
    s_gateway.config = *config;
    if (!s_gateway.config.max_retries) s_gateway.config.max_retries = 3;
    if (!s_gateway.config.retry_base_delay_ms) s_gateway.config.retry_base_delay_ms = 1000;
    if (s_gateway.config.retry_max_delay_ms < s_gateway.config.retry_base_delay_ms) s_gateway.config.retry_max_delay_ms = s_gateway.config.retry_base_delay_ms;
    clear_session(); s_gateway.retry_attempt = s_gateway.retry_delay_ms = s_gateway.retry_due_ms = 0; bump(); evaluate();
    if (s_gateway.config.auto_connect && s_gateway.config.enabled && echoear_ai_gateway_is_configured() && online() && runtime_ready()) (void)echoear_ai_gateway_request_connect();
    return echoear_ai_gateway_is_configured();
}
bool echoear_ai_gateway_set_url(const char *url) { copy_string(s_gateway.config.gateway_url, sizeof(s_gateway.config.gateway_url), url); clear_session(); s_gateway.retry_attempt = 0; bump(); evaluate(); if (s_gateway.config.auto_connect && echoear_ai_gateway_is_configured() && online() && runtime_ready()) return echoear_ai_gateway_request_connect(); return echoear_ai_gateway_is_configured(); }
bool echoear_ai_gateway_set_device_id(const char *id) { copy_string(s_gateway.device_id, sizeof(s_gateway.device_id), id); bump(); evaluate(); if (!str_nonempty(s_gateway.device_id)) return false; if (s_gateway.config.auto_connect && echoear_ai_gateway_is_configured() && online() && runtime_ready()) (void)echoear_ai_gateway_request_connect(); return true; }
void echoear_ai_gateway_set_credential_ready(bool ready) { if (s_gateway.credential_ready == ready) return; s_gateway.credential_ready = ready; bump(); if (!ready) { clear_session(); evaluate(); return; } evaluate(); if (s_gateway.config.auto_connect && echoear_ai_gateway_is_configured() && online() && runtime_ready()) (void)echoear_ai_gateway_request_connect(); }
void echoear_ai_gateway_set_connectivity(bool network_ready, bool internet_ready) { bool changed = s_gateway.network_ready != network_ready || s_gateway.internet_ready != internet_ready; s_gateway.network_ready = network_ready; s_gateway.internet_ready = internet_ready; if (changed) bump(); if (!network_ready || !internet_ready) { clear_session(); evaluate(); return; } evaluate(); if (s_gateway.config.auto_connect && echoear_ai_gateway_is_configured() && runtime_ready() && !s_gateway.session_ready && s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_BACKOFF) (void)echoear_ai_gateway_request_connect(); }

bool echoear_ai_gateway_request_connect(void) {
    if (!s_gateway.config.enabled) { set_state(ECHOEAR_AI_GATEWAY_STATE_IDLE, ECHOEAR_AI_GATEWAY_ERROR_NONE); return false; }
    if (!echoear_ai_gateway_is_configured()) { set_state(ECHOEAR_AI_GATEWAY_STATE_UNCONFIGURED, ECHOEAR_AI_GATEWAY_ERROR_NOT_CONFIGURED); return false; }
    if (!s_gateway.network_ready) { set_state(ECHOEAR_AI_GATEWAY_STATE_OFFLINE, ECHOEAR_AI_GATEWAY_ERROR_NETWORK_UNAVAILABLE); return false; }
    if (!s_gateway.internet_ready) { set_state(ECHOEAR_AI_GATEWAY_STATE_OFFLINE, ECHOEAR_AI_GATEWAY_ERROR_INTERNET_UNAVAILABLE); return false; }
    if (!str_nonempty(s_gateway.device_id)) { set_state(ECHOEAR_AI_GATEWAY_STATE_CONFIGURED, ECHOEAR_AI_GATEWAY_ERROR_DEVICE_ID_MISSING); return false; }
    if (!s_gateway.credential_ready) { set_state(ECHOEAR_AI_GATEWAY_STATE_CONFIGURED, ECHOEAR_AI_GATEWAY_ERROR_CREDENTIAL_MISSING); return false; }
    if (s_gateway.session_ready) return true;
    s_gateway.connect_request = true; set_state(ECHOEAR_AI_GATEWAY_STATE_CONNECT_REQUESTED, ECHOEAR_AI_GATEWAY_ERROR_NONE); return true;
}
bool echoear_ai_gateway_begin_connect(void) { if (!s_gateway.connect_request || s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_CONNECT_REQUESTED) return false; s_gateway.connect_request = false; s_gateway.transport_ready = s_gateway.authenticated = s_gateway.session_ready = false; s_gateway.connect_count++; set_state(ECHOEAR_AI_GATEWAY_STATE_CONNECTING, ECHOEAR_AI_GATEWAY_ERROR_NONE); return true; }
void echoear_ai_gateway_transport_connected(void) { if (s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_CONNECTING) return; s_gateway.transport_ready = true; s_gateway.auth_count++; set_state(ECHOEAR_AI_GATEWAY_STATE_AUTHENTICATING, ECHOEAR_AI_GATEWAY_ERROR_NONE); }
void echoear_ai_gateway_auth_succeeded(void) { if (s_gateway.state != ECHOEAR_AI_GATEWAY_STATE_AUTHENTICATING || !s_gateway.transport_ready) return; s_gateway.authenticated = true; s_gateway.session_ready = true; s_gateway.retry_attempt = s_gateway.retry_delay_ms = s_gateway.retry_due_ms = 0; s_gateway.session_count++; set_state(ECHOEAR_AI_GATEWAY_STATE_READY, ECHOEAR_AI_GATEWAY_ERROR_NONE); }
void echoear_ai_gateway_fail(echoear_ai_gateway_error_t error) {
    s_gateway.failure_count++; clear_session();
    if (error == ECHOEAR_AI_GATEWAY_ERROR_AUTH_FAILED || error == ECHOEAR_AI_GATEWAY_ERROR_CREDENTIAL_MISSING || error == ECHOEAR_AI_GATEWAY_ERROR_DEVICE_ID_MISSING || error == ECHOEAR_AI_GATEWAY_ERROR_INVALID_ARGUMENT || error == ECHOEAR_AI_GATEWAY_ERROR_INTERNAL) { set_state(ECHOEAR_AI_GATEWAY_STATE_ERROR, error); return; }
    if (!online()) { evaluate(); return; }
    if (retryable(error) && s_gateway.retry_attempt < s_gateway.config.max_retries) { s_gateway.retry_attempt++; s_gateway.retry_delay_ms = retry_delay(s_gateway.retry_attempt); s_gateway.retry_due_ms = s_gateway.now_ms + s_gateway.retry_delay_ms; set_state(ECHOEAR_AI_GATEWAY_STATE_BACKOFF, error); return; }
    set_state(ECHOEAR_AI_GATEWAY_STATE_DEGRADED, error);
}
void echoear_ai_gateway_disconnect(echoear_ai_gateway_error_t reason) { clear_session(); s_gateway.retry_attempt = s_gateway.retry_delay_ms = s_gateway.retry_due_ms = 0; if (!online()) { evaluate(); return; } set_state(reason == ECHOEAR_AI_GATEWAY_ERROR_NONE ? ECHOEAR_AI_GATEWAY_STATE_CONFIGURED : ECHOEAR_AI_GATEWAY_STATE_DEGRADED, reason); }
void echoear_ai_gateway_session_expired(void) { if (s_gateway.session_ready) echoear_ai_gateway_fail(ECHOEAR_AI_GATEWAY_ERROR_SESSION_EXPIRED); }
void echoear_ai_gateway_tick(uint32_t now_ms) { s_gateway.now_ms = now_ms; if (s_gateway.state == ECHOEAR_AI_GATEWAY_STATE_BACKOFF && s_gateway.retry_due_ms && now_ms >= s_gateway.retry_due_ms) { s_gateway.retry_delay_ms = s_gateway.retry_due_ms = 0; if (online() && echoear_ai_gateway_is_configured() && runtime_ready()) { s_gateway.connect_request = true; set_state(ECHOEAR_AI_GATEWAY_STATE_CONNECT_REQUESTED, ECHOEAR_AI_GATEWAY_ERROR_NONE); } else evaluate(); } }
bool echoear_ai_gateway_is_configured(void) { return !s_gateway.config.enabled || str_nonempty(s_gateway.config.gateway_url); }
bool echoear_ai_gateway_is_ready(void) { return s_gateway.state == ECHOEAR_AI_GATEWAY_STATE_READY && s_gateway.session_ready && s_gateway.authenticated && s_gateway.transport_ready; }
bool echoear_ai_gateway_should_connect(void) { return s_gateway.state == ECHOEAR_AI_GATEWAY_STATE_CONNECT_REQUESTED && s_gateway.connect_request; }

const char *echoear_ai_gateway_state_name(echoear_ai_gateway_state_t s) { switch (s) { case ECHOEAR_AI_GATEWAY_STATE_IDLE:return "idle"; case ECHOEAR_AI_GATEWAY_STATE_UNCONFIGURED:return "unconfigured"; case ECHOEAR_AI_GATEWAY_STATE_CONFIGURED:return "configured"; case ECHOEAR_AI_GATEWAY_STATE_OFFLINE:return "offline"; case ECHOEAR_AI_GATEWAY_STATE_CONNECT_REQUESTED:return "connect_requested"; case ECHOEAR_AI_GATEWAY_STATE_CONNECTING:return "connecting"; case ECHOEAR_AI_GATEWAY_STATE_AUTHENTICATING:return "authenticating"; case ECHOEAR_AI_GATEWAY_STATE_READY:return "ready"; case ECHOEAR_AI_GATEWAY_STATE_DEGRADED:return "degraded"; case ECHOEAR_AI_GATEWAY_STATE_BACKOFF:return "backoff"; case ECHOEAR_AI_GATEWAY_STATE_ERROR:return "error"; default:return "unknown"; } }
const char *echoear_ai_gateway_error_name(echoear_ai_gateway_error_t e) { switch (e) { case ECHOEAR_AI_GATEWAY_ERROR_NONE:return "none"; case ECHOEAR_AI_GATEWAY_ERROR_INVALID_ARGUMENT:return "invalid_argument"; case ECHOEAR_AI_GATEWAY_ERROR_NOT_CONFIGURED:return "not_configured"; case ECHOEAR_AI_GATEWAY_ERROR_NETWORK_UNAVAILABLE:return "network_unavailable"; case ECHOEAR_AI_GATEWAY_ERROR_INTERNET_UNAVAILABLE:return "internet_unavailable"; case ECHOEAR_AI_GATEWAY_ERROR_DEVICE_ID_MISSING:return "device_id_missing"; case ECHOEAR_AI_GATEWAY_ERROR_CREDENTIAL_MISSING:return "credential_missing"; case ECHOEAR_AI_GATEWAY_ERROR_DNS_FAILED:return "dns_failed"; case ECHOEAR_AI_GATEWAY_ERROR_CONNECT_FAILED:return "connect_failed"; case ECHOEAR_AI_GATEWAY_ERROR_TLS_FAILED:return "tls_failed"; case ECHOEAR_AI_GATEWAY_ERROR_AUTH_FAILED:return "auth_failed"; case ECHOEAR_AI_GATEWAY_ERROR_SESSION_EXPIRED:return "session_expired"; case ECHOEAR_AI_GATEWAY_ERROR_RATE_LIMITED:return "rate_limited"; case ECHOEAR_AI_GATEWAY_ERROR_SERVER_UNAVAILABLE:return "server_unavailable"; case ECHOEAR_AI_GATEWAY_ERROR_PROTOCOL:return "protocol"; case ECHOEAR_AI_GATEWAY_ERROR_INTERNAL:return "internal"; default:return "unknown"; } }
