#include "echoear_tool_router.h"

#include <string.h>

static echoear_tool_router_t s_router;

static bool text_present(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static bool copy_text(char *destination,
                      size_t destination_size,
                      const char *source)
{
    size_t length;

    if (destination == NULL ||
        destination_size == 0U ||
        source == NULL) {
        return false;
    }

    length = strlen(source);

    if (length >= destination_size) {
        return false;
    }

    memcpy(destination, source, length + 1U);
    return true;
}

static void bump_generation(void)
{
    s_router.generation++;
}

static void set_state(echoear_tool_router_state_t state,
                      echoear_tool_router_error_t error)
{
    s_router.state = state;
    s_router.error = error;
    bump_generation();
}

static void clear_active_request(void)
{
    memset(&s_router.request, 0, sizeof(s_router.request));
    memset(&s_router.active_tool, 0, sizeof(s_router.active_tool));

    s_router.request_active = false;
    s_router.confirmation_pending = false;
    s_router.dispatch_ready = false;
    s_router.executing = false;

    s_router.result[0] = '\0';
    s_router.result_length = 0U;
}

static int find_tool_index(const char *tool_name)
{
    size_t i;

    if (!text_present(tool_name)) {
        return -1;
    }

    for (i = 0U; i < s_router.tool_count; i++) {
        if (strcmp(s_router.tools[i].name, tool_name) == 0) {
            return (int)i;
        }
    }

    return -1;
}

static bool call_matches(const char *call_id)
{
    return text_present(call_id) &&
           text_present(s_router.request.call_id) &&
           strcmp(call_id, s_router.request.call_id) == 0;
}

static bool valid_risk(echoear_tool_risk_t risk)
{
    return risk >= ECHOEAR_TOOL_RISK_READ_ONLY &&
           risk < ECHOEAR_TOOL_RISK_COUNT;
}

static bool valid_policy(echoear_tool_policy_t policy)
{
    return policy >= ECHOEAR_TOOL_POLICY_AUTO_ALLOW &&
           policy < ECHOEAR_TOOL_POLICY_COUNT;
}

static bool copy_request(echoear_tool_request_t *destination,
                         const echoear_tool_request_t *source)
{
    if (destination == NULL || source == NULL) {
        return false;
    }

    memset(destination, 0, sizeof(*destination));

    return copy_text(destination->call_id,
                     sizeof(destination->call_id),
                     source->call_id) &&
           copy_text(destination->request_id,
                     sizeof(destination->request_id),
                     source->request_id) &&
           copy_text(destination->tool_name,
                     sizeof(destination->tool_name),
                     source->tool_name) &&
           copy_text(destination->arguments,
                     sizeof(destination->arguments),
                     source->arguments);
}

echoear_tool_router_config_t echoear_tool_router_default_config(void)
{
    echoear_tool_router_config_t config;

    memset(&config, 0, sizeof(config));
    config.enabled = true;

    return config;
}

void echoear_tool_router_init(void)
{
    memset(&s_router, 0, sizeof(s_router));

    s_router.config = echoear_tool_router_default_config();
    s_router.state = ECHOEAR_TOOL_ROUTER_STATE_READY;
    s_router.error = ECHOEAR_TOOL_ROUTER_ERROR_NONE;
}

void echoear_tool_router_reset(void)
{
    echoear_tool_router_init();
}

echoear_tool_router_t *echoear_tool_router_get(void)
{
    return &s_router;
}

bool echoear_tool_router_configure(
    const echoear_tool_router_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (echoear_tool_router_is_busy()) {
        return false;
    }

    s_router.config = *config;

    if (s_router.config.enabled) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_READY,
                  ECHOEAR_TOOL_ROUTER_ERROR_NONE);
    } else {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_IDLE,
                  ECHOEAR_TOOL_ROUTER_ERROR_NONE);
    }

    return true;
}

void echoear_tool_router_set_network_ready(bool ready)
{
    if (s_router.network_ready == ready) {
        return;
    }

    s_router.network_ready = ready;
    bump_generation();
}

bool echoear_tool_router_register_tool(
    const echoear_tool_descriptor_t *tool)
{
    int index;

    if (tool == NULL ||
        !text_present(tool->name) ||
        !valid_risk(tool->risk) ||
        !valid_policy(tool->policy)) {
        return false;
    }

    if (strlen(tool->name) >= ECHOEAR_TOOL_NAME_MAX) {
        return false;
    }

    if (echoear_tool_router_is_busy()) {
        return false;
    }

    index = find_tool_index(tool->name);

    if (index >= 0) {
        s_router.tools[index] = *tool;
        bump_generation();
        return true;
    }

    if (s_router.tool_count >= ECHOEAR_TOOL_ROUTER_MAX_TOOLS) {
        return false;
    }

    s_router.tools[s_router.tool_count] = *tool;
    s_router.tool_count++;

    bump_generation();
    return true;
}

bool echoear_tool_router_set_tool_enabled(
    const char *tool_name,
    bool enabled)
{
    int index;

    if (echoear_tool_router_is_busy()) {
        return false;
    }

    index = find_tool_index(tool_name);

    if (index < 0) {
        return false;
    }

    s_router.tools[index].enabled = enabled;
    bump_generation();

    return true;
}

bool echoear_tool_router_get_tool(
    const char *tool_name,
    echoear_tool_descriptor_t *tool)
{
    int index;

    if (tool == NULL) {
        return false;
    }

    index = find_tool_index(tool_name);

    if (index < 0) {
        return false;
    }

    *tool = s_router.tools[index];
    return true;
}

bool echoear_tool_router_submit(
    const echoear_tool_request_t *request)
{
    int tool_index;
    echoear_tool_descriptor_t tool;

    if (request == NULL ||
        !text_present(request->call_id) ||
        !text_present(request->request_id) ||
        !text_present(request->tool_name)) {
        if (!echoear_tool_router_is_busy()) {
            set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                      ECHOEAR_TOOL_ROUTER_ERROR_INVALID_ARGUMENT);
        }
        return false;
    }

    if (echoear_tool_router_is_busy()) {
        return false;
    }

    if (!s_router.config.enabled) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_DISABLED);
        return false;
    }

    if (strlen(request->call_id) >= ECHOEAR_TOOL_CALL_ID_MAX ||
        strlen(request->request_id) >= ECHOEAR_TOOL_REQUEST_ID_MAX ||
        strlen(request->tool_name) >= ECHOEAR_TOOL_NAME_MAX) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_INVALID_ARGUMENT);
        return false;
    }

    if (strlen(request->arguments) >= ECHOEAR_TOOL_ARGUMENTS_MAX) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_ARGUMENTS_TOO_LARGE);
        return false;
    }

    tool_index = find_tool_index(request->tool_name);

    if (tool_index < 0) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_NOT_REGISTERED);
        return false;
    }

    tool = s_router.tools[tool_index];

    if (!tool.enabled) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_TOOL_DISABLED);
        return false;
    }

    if (tool.requires_network && !s_router.network_ready) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_NETWORK_UNAVAILABLE);
        return false;
    }

    clear_active_request();

    if (!copy_request(&s_router.request, request)) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_INVALID_ARGUMENT);
        return false;
    }

    s_router.active_tool = tool;
    s_router.request_active = true;
    s_router.request_count++;

    if (tool.policy == ECHOEAR_TOOL_POLICY_DENY) {
        s_router.request_active = false;
        s_router.denied_count++;

        set_state(ECHOEAR_TOOL_ROUTER_STATE_DENIED,
                  ECHOEAR_TOOL_ROUTER_ERROR_POLICY_DENIED);

        return false;
    }

    if (tool.policy ==
        ECHOEAR_TOOL_POLICY_REQUIRE_CONFIRMATION) {
        s_router.confirmation_pending = true;

        set_state(
            ECHOEAR_TOOL_ROUTER_STATE_AWAITING_CONFIRMATION,
            ECHOEAR_TOOL_ROUTER_ERROR_NONE
        );

        return true;
    }

    s_router.dispatch_ready = true;

    set_state(ECHOEAR_TOOL_ROUTER_STATE_DISPATCH_READY,
              ECHOEAR_TOOL_ROUTER_ERROR_NONE);

    return true;
}

bool echoear_tool_router_get_pending_confirmation(
    echoear_tool_request_t *request,
    echoear_tool_descriptor_t *tool)
{
    if (s_router.state !=
            ECHOEAR_TOOL_ROUTER_STATE_AWAITING_CONFIRMATION ||
        !s_router.request_active ||
        !s_router.confirmation_pending) {
        return false;
    }

    if (request != NULL) {
        *request = s_router.request;
    }

    if (tool != NULL) {
        *tool = s_router.active_tool;
    }

    return true;
}

bool echoear_tool_router_authorize(
    const char *call_id,
    bool approved)
{
    if (!call_matches(call_id)) {
        /*
         * Stale or out-of-order authorization must never
         * corrupt the active tool call.
         */
        return false;
    }

    if (s_router.state !=
            ECHOEAR_TOOL_ROUTER_STATE_AWAITING_CONFIRMATION ||
        !s_router.request_active ||
        !s_router.confirmation_pending) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_PROTOCOL);
        return false;
    }

    s_router.confirmation_pending = false;

    if (!approved) {
        s_router.request_active = false;
        s_router.denied_count++;

        set_state(ECHOEAR_TOOL_ROUTER_STATE_DENIED,
                  ECHOEAR_TOOL_ROUTER_ERROR_USER_DENIED);

        return true;
    }

    s_router.dispatch_ready = true;

    set_state(ECHOEAR_TOOL_ROUTER_STATE_DISPATCH_READY,
              ECHOEAR_TOOL_ROUTER_ERROR_NONE);

    return true;
}

bool echoear_tool_router_take_dispatch(
    echoear_tool_dispatch_t *dispatch)
{
    if (dispatch == NULL) {
        return false;
    }

    if (s_router.state !=
            ECHOEAR_TOOL_ROUTER_STATE_DISPATCH_READY ||
        !s_router.request_active ||
        !s_router.dispatch_ready) {
        return false;
    }

    memset(dispatch, 0, sizeof(*dispatch));

    if (!copy_text(dispatch->call_id,
                   sizeof(dispatch->call_id),
                   s_router.request.call_id) ||
        !copy_text(dispatch->request_id,
                   sizeof(dispatch->request_id),
                   s_router.request.request_id) ||
        !copy_text(dispatch->tool_name,
                   sizeof(dispatch->tool_name),
                   s_router.request.tool_name) ||
        !copy_text(dispatch->arguments,
                   sizeof(dispatch->arguments),
                   s_router.request.arguments)) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_INTERNAL);
        return false;
    }

    dispatch->risk = s_router.active_tool.risk;

    s_router.dispatch_ready = false;
    s_router.executing = true;
    s_router.dispatch_count++;

    set_state(ECHOEAR_TOOL_ROUTER_STATE_EXECUTING,
              ECHOEAR_TOOL_ROUTER_ERROR_NONE);

    return true;
}

bool echoear_tool_router_complete(
    const char *call_id,
    const char *result)
{
    const char *safe_result;

    if (!call_matches(call_id)) {
        /*
         * Ignore stale execution results.
         */
        return false;
    }

    if (s_router.state !=
            ECHOEAR_TOOL_ROUTER_STATE_EXECUTING ||
        !s_router.request_active ||
        !s_router.executing) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_PROTOCOL);
        return false;
    }

    safe_result = result != NULL ? result : "";

    if (strlen(safe_result) >= ECHOEAR_TOOL_RESULT_MAX) {
        return echoear_tool_router_fail(
            call_id,
            ECHOEAR_TOOL_ROUTER_ERROR_EXECUTION_FAILED
        );
    }

    if (!copy_text(s_router.result,
                   sizeof(s_router.result),
                   safe_result)) {
        return echoear_tool_router_fail(
            call_id,
            ECHOEAR_TOOL_ROUTER_ERROR_INTERNAL
        );
    }

    s_router.result_length = strlen(s_router.result);

    s_router.request_active = false;
    s_router.executing = false;
    s_router.success_count++;

    set_state(ECHOEAR_TOOL_ROUTER_STATE_COMPLETE,
              ECHOEAR_TOOL_ROUTER_ERROR_NONE);

    return true;
}

bool echoear_tool_router_fail(
    const char *call_id,
    echoear_tool_router_error_t error)
{
    if (!call_matches(call_id)) {
        /*
         * Ignore stale execution failures.
         */
        return false;
    }

    if (!s_router.request_active ||
        s_router.state !=
            ECHOEAR_TOOL_ROUTER_STATE_EXECUTING) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR,
                  ECHOEAR_TOOL_ROUTER_ERROR_PROTOCOL);
        return false;
    }

    if (error == ECHOEAR_TOOL_ROUTER_ERROR_NONE) {
        error = ECHOEAR_TOOL_ROUTER_ERROR_EXECUTION_FAILED;
    }

    s_router.request_active = false;
    s_router.executing = false;
    s_router.dispatch_ready = false;
    s_router.confirmation_pending = false;
    s_router.failure_count++;

    set_state(ECHOEAR_TOOL_ROUTER_STATE_ERROR, error);

    return true;
}

void echoear_tool_router_cancel(void)
{
    if (!echoear_tool_router_is_busy()) {
        return;
    }

    s_router.request_active = false;
    s_router.confirmation_pending = false;
    s_router.dispatch_ready = false;
    s_router.executing = false;

    s_router.cancel_count++;

    set_state(ECHOEAR_TOOL_ROUTER_STATE_DENIED,
              ECHOEAR_TOOL_ROUTER_ERROR_CANCELLED);
}

void echoear_tool_router_clear_completed(void)
{
    if (s_router.state != ECHOEAR_TOOL_ROUTER_STATE_COMPLETE &&
        s_router.state != ECHOEAR_TOOL_ROUTER_STATE_DENIED &&
        s_router.state != ECHOEAR_TOOL_ROUTER_STATE_ERROR) {
        return;
    }

    clear_active_request();

    if (s_router.config.enabled) {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_READY,
                  ECHOEAR_TOOL_ROUTER_ERROR_NONE);
    } else {
        set_state(ECHOEAR_TOOL_ROUTER_STATE_IDLE,
                  ECHOEAR_TOOL_ROUTER_ERROR_NONE);
    }
}

bool echoear_tool_router_is_ready(void)
{
    return s_router.config.enabled &&
           s_router.state == ECHOEAR_TOOL_ROUTER_STATE_READY;
}

bool echoear_tool_router_is_busy(void)
{
    return s_router.request_active ||
           s_router.confirmation_pending ||
           s_router.dispatch_ready ||
           s_router.executing;
}

const char *echoear_tool_router_state_name(
    echoear_tool_router_state_t state)
{
    switch (state) {
        case ECHOEAR_TOOL_ROUTER_STATE_IDLE:
            return "idle";

        case ECHOEAR_TOOL_ROUTER_STATE_READY:
            return "ready";

        case ECHOEAR_TOOL_ROUTER_STATE_AWAITING_CONFIRMATION:
            return "awaiting_confirmation";

        case ECHOEAR_TOOL_ROUTER_STATE_DISPATCH_READY:
            return "dispatch_ready";

        case ECHOEAR_TOOL_ROUTER_STATE_EXECUTING:
            return "executing";

        case ECHOEAR_TOOL_ROUTER_STATE_COMPLETE:
            return "complete";

        case ECHOEAR_TOOL_ROUTER_STATE_DENIED:
            return "denied";

        case ECHOEAR_TOOL_ROUTER_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}

const char *echoear_tool_router_error_name(
    echoear_tool_router_error_t error)
{
    switch (error) {
        case ECHOEAR_TOOL_ROUTER_ERROR_NONE:
            return "none";

        case ECHOEAR_TOOL_ROUTER_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case ECHOEAR_TOOL_ROUTER_ERROR_DISABLED:
            return "disabled";

        case ECHOEAR_TOOL_ROUTER_ERROR_NOT_REGISTERED:
            return "not_registered";

        case ECHOEAR_TOOL_ROUTER_ERROR_TOOL_DISABLED:
            return "tool_disabled";

        case ECHOEAR_TOOL_ROUTER_ERROR_NETWORK_UNAVAILABLE:
            return "network_unavailable";

        case ECHOEAR_TOOL_ROUTER_ERROR_ARGUMENTS_TOO_LARGE:
            return "arguments_too_large";

        case ECHOEAR_TOOL_ROUTER_ERROR_BUSY:
            return "busy";

        case ECHOEAR_TOOL_ROUTER_ERROR_POLICY_DENIED:
            return "policy_denied";

        case ECHOEAR_TOOL_ROUTER_ERROR_USER_DENIED:
            return "user_denied";

        case ECHOEAR_TOOL_ROUTER_ERROR_STALE_CALL:
            return "stale_call";

        case ECHOEAR_TOOL_ROUTER_ERROR_EXECUTION_FAILED:
            return "execution_failed";

        case ECHOEAR_TOOL_ROUTER_ERROR_CANCELLED:
            return "cancelled";

        case ECHOEAR_TOOL_ROUTER_ERROR_PROTOCOL:
            return "protocol";

        case ECHOEAR_TOOL_ROUTER_ERROR_INTERNAL:
            return "internal";

        default:
            return "unknown";
    }
}

const char *echoear_tool_risk_name(
    echoear_tool_risk_t risk)
{
    switch (risk) {
        case ECHOEAR_TOOL_RISK_READ_ONLY:
            return "read_only";

        case ECHOEAR_TOOL_RISK_CONTROL:
            return "control";

        case ECHOEAR_TOOL_RISK_SENSITIVE:
            return "sensitive";

        default:
            return "unknown";
    }
}

const char *echoear_tool_policy_name(
    echoear_tool_policy_t policy)
{
    switch (policy) {
        case ECHOEAR_TOOL_POLICY_AUTO_ALLOW:
            return "auto_allow";

        case ECHOEAR_TOOL_POLICY_REQUIRE_CONFIRMATION:
            return "require_confirmation";

        case ECHOEAR_TOOL_POLICY_DENY:
            return "deny";

        default:
            return "unknown";
    }
}