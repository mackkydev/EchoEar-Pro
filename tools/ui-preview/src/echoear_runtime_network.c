#include "echoear_runtime_network.h"
#include <stddef.h>
#include <string.h>

static echoear_runtime_network_t s;

static void copy_text(char *dst, size_t n, const char *src)
{
    if (!dst || n == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, n - 1U);
    dst[n - 1U] = '\0';
}

static void clear_ip(void)
{
    s.ip[0] = '\0';
    s.gateway[0] = '\0';
    s.netmask[0] = '\0';
    s.associated = false;
    s.ip_ready = false;
    s.rssi = 0;
}

static bool retryable(echoear_runtime_network_error_t e)
{
    return e == ECHOEAR_RUNTIME_NETWORK_ERROR_NETWORK_NOT_FOUND ||
           e == ECHOEAR_RUNTIME_NETWORK_ERROR_DHCP_FAILED ||
           e == ECHOEAR_RUNTIME_NETWORK_ERROR_CONNECT_TIMEOUT ||
           e == ECHOEAR_RUNTIME_NETWORK_ERROR_DRIVER;
}

static uint32_t retry_delay(uint8_t attempt)
{
    uint32_t d = s.policy.retry_delay_ms;
    if (!s.policy.exponential_backoff || attempt <= 1U) return d;
    for (uint8_t i = 1U; i < attempt; ++i) {
        if (d >= s.policy.retry_delay_max_ms) return s.policy.retry_delay_max_ms;
        if (d > s.policy.retry_delay_max_ms / 2U) return s.policy.retry_delay_max_ms;
        d *= 2U;
    }
    return d > s.policy.retry_delay_max_ms ? s.policy.retry_delay_max_ms : d;
}

static void request_connect(echoear_runtime_network_reason_t reason, uint32_t now_ms)
{
    s.reason = reason;
    s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_NONE;
    s.connect_requested = true;
    s.associated = false;
    s.ip_ready = false;
    s.connect_started_ms = now_ms;
    s.retry_due_ms = 0U;
    s.current_retry_delay_ms = 0U;
    s.state = ECHOEAR_RUNTIME_NETWORK_CONNECT_REQUESTED;
    s.generation++;
}

void echoear_runtime_network_init(void)
{
    memset(&s, 0, sizeof(s));
    s.state = ECHOEAR_RUNTIME_NETWORK_IDLE;
    s.reason = ECHOEAR_RUNTIME_NETWORK_REASON_NONE;
    s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_NONE;
    s.security = ECHOEAR_WIFI_SECURITY_UNKNOWN;
    s.policy = echoear_runtime_network_default_policy();
}

void echoear_runtime_network_reset(void)
{
    uint32_t g = s.generation;
    echoear_runtime_network_init();
    s.generation = g;
}

echoear_runtime_network_t *echoear_runtime_network_get(void) { return &s; }

echoear_runtime_network_policy_t echoear_runtime_network_default_policy(void)
{
    echoear_runtime_network_policy_t p;
    p.max_boot_attempts = 3U;
    p.connect_timeout_ms = 15000U;
    p.retry_delay_ms = 1500U;
    p.retry_delay_max_ms = 6000U;
    p.exponential_backoff = true;
    return p;
}

bool echoear_runtime_network_begin_boot(bool setup_completed,
                                        bool credentials_available,
                                        const char *ssid,
                                        echoear_wifi_security_t security,
                                        uint32_t now_ms)
{
    echoear_runtime_network_reset();
    s.state = ECHOEAR_RUNTIME_NETWORK_BOOT_CHECK;
    s.reason = ECHOEAR_RUNTIME_NETWORK_REASON_BOOT;
    s.setup_completed = setup_completed;
    s.credentials_available = credentials_available;
    s.security = security;
    copy_text(s.ssid, sizeof(s.ssid), ssid);
    s.generation++;

    if (!setup_completed || !credentials_available) {
        s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_CREDENTIALS_MISSING;
        s.provisioning_required = true;
        s.state = ECHOEAR_RUNTIME_NETWORK_PROVISIONING_REQUIRED;
        return false;
    }

    if (s.ssid[0] == '\0') {
        s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_INVALID_SSID;
        s.provisioning_required = true;
        s.state = ECHOEAR_RUNTIME_NETWORK_PROVISIONING_REQUIRED;
        return false;
    }

    s.provisioning_required = false;
    s.attempt = 1U;
    request_connect(ECHOEAR_RUNTIME_NETWORK_REASON_BOOT, now_ms);
    return true;
}

bool echoear_runtime_network_take_connect_request(void)
{
    bool v = s.connect_requested;
    s.connect_requested = false;
    return v;
}

void echoear_runtime_network_mark_connecting(uint32_t now_ms)
{
    clear_ip();
    s.connect_requested = false;
    s.connect_started_ms = now_ms;
    s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_NONE;
    s.state = ECHOEAR_RUNTIME_NETWORK_CONNECTING;
    s.generation++;
}

void echoear_runtime_network_mark_associated(int16_t rssi)
{
    s.associated = true;
    s.ip_ready = false;
    s.rssi = rssi;
    s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_NONE;
    s.state = ECHOEAR_RUNTIME_NETWORK_WAITING_IP;
    s.generation++;
}

void echoear_runtime_network_mark_ip_ready(const char *ip,
                                           const char *gateway,
                                           const char *netmask,
                                           int16_t rssi)
{
    copy_text(s.ip, sizeof(s.ip), ip);
    copy_text(s.gateway, sizeof(s.gateway), gateway);
    copy_text(s.netmask, sizeof(s.netmask), netmask);
    s.associated = true;
    s.ip_ready = s.ip[0] != '\0';
    s.rssi = rssi;
    s.connect_requested = false;
    s.provisioning_required = false;
    s.current_retry_delay_ms = 0U;
    s.retry_due_ms = 0U;
    if (s.ip_ready) {
        s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_NONE;
        s.state = ECHOEAR_RUNTIME_NETWORK_READY;
    } else {
        s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_DHCP_FAILED;
        s.state = ECHOEAR_RUNTIME_NETWORK_ERROR;
    }
    s.generation++;
}

void echoear_runtime_network_mark_offline(echoear_runtime_network_error_t error)
{
    clear_ip();
    s.connect_requested = false;
    s.error = error;
    s.state = ECHOEAR_RUNTIME_NETWORK_OFFLINE;
    s.provisioning_required = false;
    s.generation++;
}

void echoear_runtime_network_report_connect_failure(echoear_runtime_network_error_t error,
                                                     uint32_t now_ms)
{
    clear_ip();
    s.connect_requested = false;
    s.error = error;

    if (error == ECHOEAR_RUNTIME_NETWORK_ERROR_AUTH_FAILED) {
        s.state = ECHOEAR_RUNTIME_NETWORK_OFFLINE;
        s.provisioning_required = false;
        s.generation++;
        return;
    }

    if (!retryable(error)) {
        s.state = ECHOEAR_RUNTIME_NETWORK_ERROR;
        s.generation++;
        return;
    }

    if (s.attempt >= s.policy.max_boot_attempts) {
        s.state = ECHOEAR_RUNTIME_NETWORK_OFFLINE;
        s.provisioning_required = false;
        s.generation++;
        return;
    }

    s.current_retry_delay_ms = retry_delay(s.attempt);
    s.retry_due_ms = now_ms + s.current_retry_delay_ms;
    s.state = ECHOEAR_RUNTIME_NETWORK_RETRY_WAIT;
    s.generation++;
}

void echoear_runtime_network_tick(uint32_t now_ms)
{
    if ((s.state == ECHOEAR_RUNTIME_NETWORK_CONNECTING ||
         s.state == ECHOEAR_RUNTIME_NETWORK_WAITING_IP) &&
        s.policy.connect_timeout_ms > 0U &&
        (uint32_t)(now_ms - s.connect_started_ms) >= s.policy.connect_timeout_ms) {
        echoear_runtime_network_report_connect_failure(
            ECHOEAR_RUNTIME_NETWORK_ERROR_CONNECT_TIMEOUT, now_ms);
        return;
    }

    if (s.state != ECHOEAR_RUNTIME_NETWORK_RETRY_WAIT) return;
    if ((int32_t)(now_ms - s.retry_due_ms) < 0) return;

    s.attempt++;
    request_connect(ECHOEAR_RUNTIME_NETWORK_REASON_RETRY, now_ms);
}

void echoear_runtime_network_request_manual_connect(uint32_t now_ms)
{
    if (!s.credentials_available || s.ssid[0] == '\0') {
        s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_CREDENTIALS_MISSING;
        s.provisioning_required = true;
        s.state = ECHOEAR_RUNTIME_NETWORK_PROVISIONING_REQUIRED;
        s.generation++;
        return;
    }
    s.attempt = 1U;
    request_connect(ECHOEAR_RUNTIME_NETWORK_REASON_MANUAL, now_ms);
}

void echoear_runtime_network_clear_error(void)
{
    s.error = ECHOEAR_RUNTIME_NETWORK_ERROR_NONE;
    if (s.state == ECHOEAR_RUNTIME_NETWORK_ERROR) s.state = ECHOEAR_RUNTIME_NETWORK_IDLE;
    s.generation++;
}

bool echoear_runtime_network_is_ready(void) { return s.state == ECHOEAR_RUNTIME_NETWORK_READY && s.ip_ready; }
bool echoear_runtime_network_needs_provisioning(void) { return s.provisioning_required; }
bool echoear_runtime_network_is_connecting(void)
{
    return s.state == ECHOEAR_RUNTIME_NETWORK_CONNECT_REQUESTED ||
           s.state == ECHOEAR_RUNTIME_NETWORK_CONNECTING ||
           s.state == ECHOEAR_RUNTIME_NETWORK_WAITING_IP ||
           s.state == ECHOEAR_RUNTIME_NETWORK_RETRY_WAIT;
}
bool echoear_runtime_network_is_terminal_error(void)
{
    return s.state == ECHOEAR_RUNTIME_NETWORK_ERROR || s.state == ECHOEAR_RUNTIME_NETWORK_OFFLINE;
}

bool echoear_runtime_network_parse_state(const char *v, echoear_runtime_network_state_t *out)
{
    if (!v || !out) return false;
    if (!strcmp(v,"idle")) *out=ECHOEAR_RUNTIME_NETWORK_IDLE;
    else if (!strcmp(v,"boot_check")) *out=ECHOEAR_RUNTIME_NETWORK_BOOT_CHECK;
    else if (!strcmp(v,"provisioning_required")) *out=ECHOEAR_RUNTIME_NETWORK_PROVISIONING_REQUIRED;
    else if (!strcmp(v,"connect_requested")) *out=ECHOEAR_RUNTIME_NETWORK_CONNECT_REQUESTED;
    else if (!strcmp(v,"connecting")) *out=ECHOEAR_RUNTIME_NETWORK_CONNECTING;
    else if (!strcmp(v,"waiting_ip")) *out=ECHOEAR_RUNTIME_NETWORK_WAITING_IP;
    else if (!strcmp(v,"ready")) *out=ECHOEAR_RUNTIME_NETWORK_READY;
    else if (!strcmp(v,"retry_wait")) *out=ECHOEAR_RUNTIME_NETWORK_RETRY_WAIT;
    else if (!strcmp(v,"offline")) *out=ECHOEAR_RUNTIME_NETWORK_OFFLINE;
    else if (!strcmp(v,"error")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR;
    else return false;
    return true;
}

bool echoear_runtime_network_parse_reason(const char *v, echoear_runtime_network_reason_t *out)
{
    if (!v || !out) return false;
    if (!strcmp(v,"none")) *out=ECHOEAR_RUNTIME_NETWORK_REASON_NONE;
    else if (!strcmp(v,"boot")) *out=ECHOEAR_RUNTIME_NETWORK_REASON_BOOT;
    else if (!strcmp(v,"retry")) *out=ECHOEAR_RUNTIME_NETWORK_REASON_RETRY;
    else if (!strcmp(v,"runtime_reconnect")) *out=ECHOEAR_RUNTIME_NETWORK_REASON_RUNTIME_RECONNECT;
    else if (!strcmp(v,"manual")) *out=ECHOEAR_RUNTIME_NETWORK_REASON_MANUAL;
    else return false;
    return true;
}

bool echoear_runtime_network_parse_error(const char *v, echoear_runtime_network_error_t *out)
{
    if (!v || !out) return false;
    if (!strcmp(v,"none") || !v[0]) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_NONE;
    else if (!strcmp(v,"credentials_missing")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_CREDENTIALS_MISSING;
    else if (!strcmp(v,"invalid_ssid")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_INVALID_SSID;
    else if (!strcmp(v,"network_not_found")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_NETWORK_NOT_FOUND;
    else if (!strcmp(v,"auth_failed")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_AUTH_FAILED;
    else if (!strcmp(v,"dhcp_failed")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_DHCP_FAILED;
    else if (!strcmp(v,"connect_timeout")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_CONNECT_TIMEOUT;
    else if (!strcmp(v,"driver")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_DRIVER;
    else if (!strcmp(v,"internal")) *out=ECHOEAR_RUNTIME_NETWORK_ERROR_INTERNAL;
    else return false;
    return true;
}

const char *echoear_runtime_network_state_name(echoear_runtime_network_state_t v)
{
    switch(v){
    case ECHOEAR_RUNTIME_NETWORK_IDLE:return "idle";
    case ECHOEAR_RUNTIME_NETWORK_BOOT_CHECK:return "boot_check";
    case ECHOEAR_RUNTIME_NETWORK_PROVISIONING_REQUIRED:return "provisioning_required";
    case ECHOEAR_RUNTIME_NETWORK_CONNECT_REQUESTED:return "connect_requested";
    case ECHOEAR_RUNTIME_NETWORK_CONNECTING:return "connecting";
    case ECHOEAR_RUNTIME_NETWORK_WAITING_IP:return "waiting_ip";
    case ECHOEAR_RUNTIME_NETWORK_READY:return "ready";
    case ECHOEAR_RUNTIME_NETWORK_RETRY_WAIT:return "retry_wait";
    case ECHOEAR_RUNTIME_NETWORK_OFFLINE:return "offline";
    default:return "error"; }
}
const char *echoear_runtime_network_reason_name(echoear_runtime_network_reason_t v)
{
    switch(v){
    case ECHOEAR_RUNTIME_NETWORK_REASON_BOOT:return "boot";
    case ECHOEAR_RUNTIME_NETWORK_REASON_RETRY:return "retry";
    case ECHOEAR_RUNTIME_NETWORK_REASON_RUNTIME_RECONNECT:return "runtime_reconnect";
    case ECHOEAR_RUNTIME_NETWORK_REASON_MANUAL:return "manual";
    default:return "none"; }
}
const char *echoear_runtime_network_error_name(echoear_runtime_network_error_t v)
{
    switch(v){
    case ECHOEAR_RUNTIME_NETWORK_ERROR_NONE:return "none";
    case ECHOEAR_RUNTIME_NETWORK_ERROR_CREDENTIALS_MISSING:return "credentials_missing";
    case ECHOEAR_RUNTIME_NETWORK_ERROR_INVALID_SSID:return "invalid_ssid";
    case ECHOEAR_RUNTIME_NETWORK_ERROR_NETWORK_NOT_FOUND:return "network_not_found";
    case ECHOEAR_RUNTIME_NETWORK_ERROR_AUTH_FAILED:return "auth_failed";
    case ECHOEAR_RUNTIME_NETWORK_ERROR_DHCP_FAILED:return "dhcp_failed";
    case ECHOEAR_RUNTIME_NETWORK_ERROR_CONNECT_TIMEOUT:return "connect_timeout";
    case ECHOEAR_RUNTIME_NETWORK_ERROR_DRIVER:return "driver";
    default:return "internal"; }
}
