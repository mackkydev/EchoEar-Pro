#ifndef ECHOEAR_RUNTIME_NETWORK_H
#define ECHOEAR_RUNTIME_NETWORK_H

#include <stdbool.h>
#include <stdint.h>
#include "echoear_wifi_manager.h"

#define ECHOEAR_RUNTIME_NETWORK_IPV4_MAX 16

typedef enum {
    ECHOEAR_RUNTIME_NETWORK_IDLE = 0,
    ECHOEAR_RUNTIME_NETWORK_BOOT_CHECK,
    ECHOEAR_RUNTIME_NETWORK_PROVISIONING_REQUIRED,
    ECHOEAR_RUNTIME_NETWORK_CONNECT_REQUESTED,
    ECHOEAR_RUNTIME_NETWORK_CONNECTING,
    ECHOEAR_RUNTIME_NETWORK_WAITING_IP,
    ECHOEAR_RUNTIME_NETWORK_READY,
    ECHOEAR_RUNTIME_NETWORK_RETRY_WAIT,
    ECHOEAR_RUNTIME_NETWORK_OFFLINE,
    ECHOEAR_RUNTIME_NETWORK_ERROR
} echoear_runtime_network_state_t;

typedef enum {
    ECHOEAR_RUNTIME_NETWORK_REASON_NONE = 0,
    ECHOEAR_RUNTIME_NETWORK_REASON_BOOT,
    ECHOEAR_RUNTIME_NETWORK_REASON_RETRY,
    ECHOEAR_RUNTIME_NETWORK_REASON_RUNTIME_RECONNECT,
    ECHOEAR_RUNTIME_NETWORK_REASON_MANUAL
} echoear_runtime_network_reason_t;

typedef enum {
    ECHOEAR_RUNTIME_NETWORK_ERROR_NONE = 0,
    ECHOEAR_RUNTIME_NETWORK_ERROR_CREDENTIALS_MISSING,
    ECHOEAR_RUNTIME_NETWORK_ERROR_INVALID_SSID,
    ECHOEAR_RUNTIME_NETWORK_ERROR_NETWORK_NOT_FOUND,
    ECHOEAR_RUNTIME_NETWORK_ERROR_AUTH_FAILED,
    ECHOEAR_RUNTIME_NETWORK_ERROR_DHCP_FAILED,
    ECHOEAR_RUNTIME_NETWORK_ERROR_CONNECT_TIMEOUT,
    ECHOEAR_RUNTIME_NETWORK_ERROR_DRIVER,
    ECHOEAR_RUNTIME_NETWORK_ERROR_INTERNAL
} echoear_runtime_network_error_t;

typedef struct {
    uint8_t max_boot_attempts;
    uint32_t connect_timeout_ms;
    uint32_t retry_delay_ms;
    uint32_t retry_delay_max_ms;
    bool exponential_backoff;
} echoear_runtime_network_policy_t;

typedef struct {
    echoear_runtime_network_state_t state;
    echoear_runtime_network_reason_t reason;
    echoear_runtime_network_error_t error;
    echoear_runtime_network_policy_t policy;
    bool setup_completed;
    bool credentials_available;
    bool provisioning_required;
    bool connect_requested;
    bool associated;
    bool ip_ready;
    char ssid[ECHOEAR_WIFI_SSID_MAX];
    echoear_wifi_security_t security;
    char ip[ECHOEAR_RUNTIME_NETWORK_IPV4_MAX];
    char gateway[ECHOEAR_RUNTIME_NETWORK_IPV4_MAX];
    char netmask[ECHOEAR_RUNTIME_NETWORK_IPV4_MAX];
    int16_t rssi;
    uint8_t attempt;
    uint32_t connect_started_ms;
    uint32_t retry_due_ms;
    uint32_t current_retry_delay_ms;
    uint32_t generation;
} echoear_runtime_network_t;

void echoear_runtime_network_init(void);
void echoear_runtime_network_reset(void);
echoear_runtime_network_t *echoear_runtime_network_get(void);
echoear_runtime_network_policy_t echoear_runtime_network_default_policy(void);

bool echoear_runtime_network_begin_boot(bool setup_completed,
                                        bool credentials_available,
                                        const char *ssid,
                                        echoear_wifi_security_t security,
                                        uint32_t now_ms);

bool echoear_runtime_network_take_connect_request(void);
void echoear_runtime_network_mark_connecting(uint32_t now_ms);
void echoear_runtime_network_mark_associated(int16_t rssi);
void echoear_runtime_network_mark_ip_ready(const char *ip,
                                           const char *gateway,
                                           const char *netmask,
                                           int16_t rssi);
void echoear_runtime_network_mark_offline(echoear_runtime_network_error_t error);
void echoear_runtime_network_report_connect_failure(echoear_runtime_network_error_t error,
                                                     uint32_t now_ms);
void echoear_runtime_network_tick(uint32_t now_ms);
void echoear_runtime_network_request_manual_connect(uint32_t now_ms);
void echoear_runtime_network_clear_error(void);

bool echoear_runtime_network_is_ready(void);
bool echoear_runtime_network_needs_provisioning(void);
bool echoear_runtime_network_is_connecting(void);
bool echoear_runtime_network_is_terminal_error(void);

bool echoear_runtime_network_parse_state(const char *value,
                                         echoear_runtime_network_state_t *state);
bool echoear_runtime_network_parse_reason(const char *value,
                                          echoear_runtime_network_reason_t *reason);
bool echoear_runtime_network_parse_error(const char *value,
                                         echoear_runtime_network_error_t *error);
const char *echoear_runtime_network_state_name(echoear_runtime_network_state_t state);
const char *echoear_runtime_network_reason_name(echoear_runtime_network_reason_t reason);
const char *echoear_runtime_network_error_name(echoear_runtime_network_error_t error);

#endif
