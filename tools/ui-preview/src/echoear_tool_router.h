#ifndef ECHOEAR_TOOL_ROUTER_H
#define ECHOEAR_TOOL_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_TOOL_ROUTER_MAX_TOOLS       16
#define ECHOEAR_TOOL_NAME_MAX               64
#define ECHOEAR_TOOL_CALL_ID_MAX            64
#define ECHOEAR_TOOL_REQUEST_ID_MAX         64
#define ECHOEAR_TOOL_ARGUMENTS_MAX          768
#define ECHOEAR_TOOL_RESULT_MAX             768

typedef enum {
    ECHOEAR_TOOL_RISK_READ_ONLY = 0,
    ECHOEAR_TOOL_RISK_CONTROL,
    ECHOEAR_TOOL_RISK_SENSITIVE,
    ECHOEAR_TOOL_RISK_COUNT
} echoear_tool_risk_t;

typedef enum {
    ECHOEAR_TOOL_POLICY_AUTO_ALLOW = 0,
    ECHOEAR_TOOL_POLICY_REQUIRE_CONFIRMATION,
    ECHOEAR_TOOL_POLICY_DENY,
    ECHOEAR_TOOL_POLICY_COUNT
} echoear_tool_policy_t;

typedef enum {
    ECHOEAR_TOOL_ROUTER_STATE_IDLE = 0,
    ECHOEAR_TOOL_ROUTER_STATE_READY,
    ECHOEAR_TOOL_ROUTER_STATE_AWAITING_CONFIRMATION,
    ECHOEAR_TOOL_ROUTER_STATE_DISPATCH_READY,
    ECHOEAR_TOOL_ROUTER_STATE_EXECUTING,
    ECHOEAR_TOOL_ROUTER_STATE_COMPLETE,
    ECHOEAR_TOOL_ROUTER_STATE_DENIED,
    ECHOEAR_TOOL_ROUTER_STATE_ERROR
} echoear_tool_router_state_t;

typedef enum {
    ECHOEAR_TOOL_ROUTER_ERROR_NONE = 0,
    ECHOEAR_TOOL_ROUTER_ERROR_INVALID_ARGUMENT,
    ECHOEAR_TOOL_ROUTER_ERROR_DISABLED,
    ECHOEAR_TOOL_ROUTER_ERROR_NOT_REGISTERED,
    ECHOEAR_TOOL_ROUTER_ERROR_TOOL_DISABLED,
    ECHOEAR_TOOL_ROUTER_ERROR_NETWORK_UNAVAILABLE,
    ECHOEAR_TOOL_ROUTER_ERROR_ARGUMENTS_TOO_LARGE,
    ECHOEAR_TOOL_ROUTER_ERROR_BUSY,
    ECHOEAR_TOOL_ROUTER_ERROR_POLICY_DENIED,
    ECHOEAR_TOOL_ROUTER_ERROR_USER_DENIED,
    ECHOEAR_TOOL_ROUTER_ERROR_STALE_CALL,
    ECHOEAR_TOOL_ROUTER_ERROR_EXECUTION_FAILED,
    ECHOEAR_TOOL_ROUTER_ERROR_CANCELLED,
    ECHOEAR_TOOL_ROUTER_ERROR_PROTOCOL,
    ECHOEAR_TOOL_ROUTER_ERROR_INTERNAL
} echoear_tool_router_error_t;

typedef struct {
    bool enabled;
} echoear_tool_router_config_t;

typedef struct {
    char name[ECHOEAR_TOOL_NAME_MAX];

    echoear_tool_risk_t risk;
    echoear_tool_policy_t policy;

    bool enabled;
    bool requires_network;
} echoear_tool_descriptor_t;

typedef struct {
    char call_id[ECHOEAR_TOOL_CALL_ID_MAX];
    char request_id[ECHOEAR_TOOL_REQUEST_ID_MAX];
    char tool_name[ECHOEAR_TOOL_NAME_MAX];
    char arguments[ECHOEAR_TOOL_ARGUMENTS_MAX];
} echoear_tool_request_t;

typedef struct {
    char call_id[ECHOEAR_TOOL_CALL_ID_MAX];
    char request_id[ECHOEAR_TOOL_REQUEST_ID_MAX];
    char tool_name[ECHOEAR_TOOL_NAME_MAX];
    char arguments[ECHOEAR_TOOL_ARGUMENTS_MAX];

    echoear_tool_risk_t risk;
} echoear_tool_dispatch_t;

typedef struct {
    echoear_tool_router_state_t state;
    echoear_tool_router_error_t error;
    echoear_tool_router_config_t config;

    bool network_ready;

    bool request_active;
    bool confirmation_pending;
    bool dispatch_ready;
    bool executing;

    echoear_tool_request_t request;
    echoear_tool_descriptor_t active_tool;

    char result[ECHOEAR_TOOL_RESULT_MAX];
    size_t result_length;

    echoear_tool_descriptor_t tools[ECHOEAR_TOOL_ROUTER_MAX_TOOLS];
    size_t tool_count;

    uint32_t request_count;
    uint32_t dispatch_count;
    uint32_t success_count;
    uint32_t failure_count;
    uint32_t denied_count;
    uint32_t cancel_count;

    uint32_t generation;
} echoear_tool_router_t;

echoear_tool_router_config_t echoear_tool_router_default_config(void);

void echoear_tool_router_init(void);
void echoear_tool_router_reset(void);

echoear_tool_router_t *echoear_tool_router_get(void);

bool echoear_tool_router_configure(
    const echoear_tool_router_config_t *config
);

void echoear_tool_router_set_network_ready(bool ready);

bool echoear_tool_router_register_tool(
    const echoear_tool_descriptor_t *tool
);

bool echoear_tool_router_set_tool_enabled(
    const char *tool_name,
    bool enabled
);

bool echoear_tool_router_get_tool(
    const char *tool_name,
    echoear_tool_descriptor_t *tool
);

bool echoear_tool_router_submit(
    const echoear_tool_request_t *request
);

bool echoear_tool_router_get_pending_confirmation(
    echoear_tool_request_t *request,
    echoear_tool_descriptor_t *tool
);

bool echoear_tool_router_authorize(
    const char *call_id,
    bool approved
);

bool echoear_tool_router_take_dispatch(
    echoear_tool_dispatch_t *dispatch
);

bool echoear_tool_router_complete(
    const char *call_id,
    const char *result
);

bool echoear_tool_router_fail(
    const char *call_id,
    echoear_tool_router_error_t error
);

void echoear_tool_router_cancel(void);
void echoear_tool_router_clear_completed(void);

bool echoear_tool_router_is_ready(void);
bool echoear_tool_router_is_busy(void);

const char *echoear_tool_router_state_name(
    echoear_tool_router_state_t state
);

const char *echoear_tool_router_error_name(
    echoear_tool_router_error_t error
);

const char *echoear_tool_risk_name(
    echoear_tool_risk_t risk
);

const char *echoear_tool_policy_name(
    echoear_tool_policy_t policy
);

#ifdef __cplusplus
}
#endif

#endif