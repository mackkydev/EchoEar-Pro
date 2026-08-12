#include "echoear_conversation.h"

#include <string.h>

static echoear_conversation_t s_conversation;

static bool copy_text(char *dst, size_t dst_size, const char *src)
{
    size_t len;
    if (dst == NULL || dst_size == 0U || src == NULL) return false;
    len = strlen(src);
    if (len >= dst_size) return false;
    memcpy(dst, src, len + 1U);
    return true;
}

static bool text_present(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static bool request_matches(const char *request_id)
{
    return text_present(request_id) &&
           strcmp(s_conversation.request_id, request_id) == 0;
}

static void bump_generation(void)
{
    s_conversation.generation++;
}

static void clear_turn_buffers(void)
{
    s_conversation.request_active = false;
    s_conversation.response_active = false;
    s_conversation.request_id[0] = '\0';
    s_conversation.user_text[0] = '\0';
    s_conversation.assistant_text[0] = '\0';
    s_conversation.assistant_text_length = 0U;
    s_conversation.deadline_ms = 0U;
}

static void set_state(echoear_conversation_state_t state,
                      echoear_conversation_error_t error)
{
    if (s_conversation.state != state || s_conversation.error != error) {
        s_conversation.state = state;
        s_conversation.error = error;
        bump_generation();
    }
}

static void fail_active_turn(echoear_conversation_error_t error)
{
    if (s_conversation.request_active || s_conversation.response_active) {
        s_conversation.failure_count++;
    }
    s_conversation.request_active = false;
    s_conversation.response_active = false;
    s_conversation.deadline_ms = 0U;
    set_state(ECHOEAR_CONVERSATION_STATE_ERROR, error);
}

echoear_conversation_config_t echoear_conversation_default_config(void)
{
    echoear_conversation_config_t config;
    memset(&config, 0, sizeof(config));
    config.enabled = true;
    config.request_timeout_ms = 15000U;
    config.response_timeout_ms = 60000U;
    (void)copy_text(config.locale, sizeof(config.locale), "th-TH");
    return config;
}

void echoear_conversation_init(void)
{
    memset(&s_conversation, 0, sizeof(s_conversation));
    s_conversation.config = echoear_conversation_default_config();
    set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
              ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
}

void echoear_conversation_reset(void)
{
    echoear_conversation_init();
}

echoear_conversation_t *echoear_conversation_get(void)
{
    return &s_conversation;
}

bool echoear_conversation_configure(const echoear_conversation_config_t *config)
{
    if (config == NULL ||
        config->request_timeout_ms == 0U ||
        config->response_timeout_ms == 0U ||
        !text_present(config->locale) ||
        strlen(config->locale) >= ECHOEAR_CONVERSATION_LOCALE_MAX) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_INVALID_ARGUMENT);
        return false;
    }

    s_conversation.config = *config;
    bump_generation();

    if (!config->enabled) {
        clear_turn_buffers();
        set_state(ECHOEAR_CONVERSATION_STATE_IDLE,
                  ECHOEAR_CONVERSATION_ERROR_DISABLED);
    } else if (s_conversation.gateway_ready) {
        set_state(ECHOEAR_CONVERSATION_STATE_READY,
                  ECHOEAR_CONVERSATION_ERROR_NONE);
    } else {
        set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
                  ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
    }
    return true;
}

void echoear_conversation_set_gateway_ready(bool ready)
{
    if (s_conversation.gateway_ready == ready) return;

    s_conversation.gateway_ready = ready;
    bump_generation();

    if (!s_conversation.config.enabled) {
        set_state(ECHOEAR_CONVERSATION_STATE_IDLE,
                  ECHOEAR_CONVERSATION_ERROR_DISABLED);
        return;
    }

    if (!ready) {
        if (s_conversation.request_active || s_conversation.response_active) {
            s_conversation.failure_count++;
            s_conversation.request_active = false;
            s_conversation.response_active = false;
            s_conversation.deadline_ms = 0U;
        }
        set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
                  ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
        return;
    }

    set_state(ECHOEAR_CONVERSATION_STATE_READY,
              ECHOEAR_CONVERSATION_ERROR_NONE);
}

bool echoear_conversation_start_session(const char *conversation_id)
{
    if (!text_present(conversation_id) ||
        strlen(conversation_id) >= ECHOEAR_CONVERSATION_ID_MAX) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_INVALID_ARGUMENT);
        return false;
    }

    if (echoear_conversation_is_busy()) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_BUSY);
        return false;
    }

    if (!copy_text(s_conversation.conversation_id,
                   sizeof(s_conversation.conversation_id),
                   conversation_id)) {
        return false;
    }

    s_conversation.session_active = true;
    s_conversation.turn_count = 0U;
    clear_turn_buffers();
    bump_generation();

    if (s_conversation.gateway_ready) {
        set_state(ECHOEAR_CONVERSATION_STATE_READY,
                  ECHOEAR_CONVERSATION_ERROR_NONE);
    } else {
        set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
                  ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
    }
    return true;
}

void echoear_conversation_end_session(void)
{
    clear_turn_buffers();
    s_conversation.session_active = false;
    s_conversation.conversation_id[0] = '\0';
    s_conversation.turn_count = 0U;
    bump_generation();

    if (!s_conversation.config.enabled) {
        set_state(ECHOEAR_CONVERSATION_STATE_IDLE,
                  ECHOEAR_CONVERSATION_ERROR_DISABLED);
    } else if (s_conversation.gateway_ready) {
        set_state(ECHOEAR_CONVERSATION_STATE_READY,
                  ECHOEAR_CONVERSATION_ERROR_NONE);
    } else {
        set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
                  ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
    }
}

bool echoear_conversation_submit_user_text(const char *request_id,
                                           const char *text)
{
    if (!s_conversation.config.enabled) {
        set_state(ECHOEAR_CONVERSATION_STATE_IDLE,
                  ECHOEAR_CONVERSATION_ERROR_DISABLED);
        return false;
    }
    if (!s_conversation.gateway_ready) {
        set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
                  ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
        return false;
    }
    if (!s_conversation.session_active) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_SESSION_REQUIRED);
        return false;
    }
    if (echoear_conversation_is_busy()) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_BUSY);
        return false;
    }
    if (!text_present(request_id) ||
        strlen(request_id) >= ECHOEAR_CONVERSATION_REQUEST_ID_MAX ||
        !text_present(text) ||
        strlen(text) >= ECHOEAR_CONVERSATION_TEXT_MAX) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_INVALID_ARGUMENT);
        return false;
    }

    clear_turn_buffers();

    if (!copy_text(s_conversation.request_id,
                   sizeof(s_conversation.request_id), request_id) ||
        !copy_text(s_conversation.user_text,
                   sizeof(s_conversation.user_text), text)) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_INVALID_ARGUMENT);
        return false;
    }

    s_conversation.request_active = true;
    s_conversation.request_count++;
    s_conversation.deadline_ms =
        s_conversation.now_ms + s_conversation.config.request_timeout_ms;

    set_state(ECHOEAR_CONVERSATION_STATE_REQUEST_PENDING,
              ECHOEAR_CONVERSATION_ERROR_NONE);
    return true;
}

bool echoear_conversation_get_pending_request(echoear_conversation_request_t *request)
{
    if (request == NULL ||
        s_conversation.state != ECHOEAR_CONVERSATION_STATE_REQUEST_PENDING ||
        !s_conversation.request_active) return false;

    memset(request, 0, sizeof(*request));

    return copy_text(request->conversation_id, sizeof(request->conversation_id),
                     s_conversation.conversation_id) &&
           copy_text(request->request_id, sizeof(request->request_id),
                     s_conversation.request_id) &&
           copy_text(request->locale, sizeof(request->locale),
                     s_conversation.config.locale) &&
           copy_text(request->text, sizeof(request->text),
                     s_conversation.user_text);
}

bool echoear_conversation_mark_request_dispatched(const char *request_id)
{
    if (!request_matches(request_id)) {
        return false;
    }

    if (s_conversation.state != ECHOEAR_CONVERSATION_STATE_REQUEST_PENDING ||
        !s_conversation.request_active) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_PROTOCOL);
        return false;
    }

    s_conversation.deadline_ms =
        s_conversation.now_ms + s_conversation.config.response_timeout_ms;
    set_state(ECHOEAR_CONVERSATION_STATE_WAITING_RESPONSE,
              ECHOEAR_CONVERSATION_ERROR_NONE);
    return true;
}

bool echoear_conversation_response_begin(const char *request_id)
{
    if (!request_matches(request_id)) {
        return false;
    }

    if (!s_conversation.request_active ||
        s_conversation.state != ECHOEAR_CONVERSATION_STATE_WAITING_RESPONSE) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_PROTOCOL);
        return false;
    }

    s_conversation.response_active = true;
    s_conversation.assistant_text[0] = '\0';
    s_conversation.assistant_text_length = 0U;
    s_conversation.deadline_ms =
        s_conversation.now_ms + s_conversation.config.response_timeout_ms;

    set_state(ECHOEAR_CONVERSATION_STATE_STREAMING_RESPONSE,
              ECHOEAR_CONVERSATION_ERROR_NONE);
    return true;
}

bool echoear_conversation_response_append(const char *request_id,
                                          const char *chunk)
{
    size_t chunk_length;
    size_t remaining;

    if (!request_matches(request_id)) {
        return false;
    }

    if (!s_conversation.request_active ||
        !s_conversation.response_active ||
        s_conversation.state != ECHOEAR_CONVERSATION_STATE_STREAMING_RESPONSE) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_PROTOCOL);
        return false;
    }

    if (!text_present(chunk)) return true;

    chunk_length = strlen(chunk);
    remaining = ECHOEAR_CONVERSATION_TEXT_MAX -
                s_conversation.assistant_text_length - 1U;

    if (chunk_length > remaining) {
        fail_active_turn(ECHOEAR_CONVERSATION_ERROR_RESPONSE_TOO_LARGE);
        return false;
    }

    memcpy(s_conversation.assistant_text +
               s_conversation.assistant_text_length,
           chunk, chunk_length + 1U);

    s_conversation.assistant_text_length += chunk_length;
    s_conversation.deadline_ms =
        s_conversation.now_ms + s_conversation.config.response_timeout_ms;
    bump_generation();
    return true;
}

bool echoear_conversation_response_complete(const char *request_id)
{
    if (!request_matches(request_id)) {
        return false;
    }

    if (!s_conversation.request_active ||
        !s_conversation.response_active ||
        s_conversation.state != ECHOEAR_CONVERSATION_STATE_STREAMING_RESPONSE) {
        set_state(ECHOEAR_CONVERSATION_STATE_ERROR,
                  ECHOEAR_CONVERSATION_ERROR_PROTOCOL);
        return false;
    }

    s_conversation.request_active = false;
    s_conversation.response_active = false;
    s_conversation.deadline_ms = 0U;
    s_conversation.turn_count++;
    s_conversation.success_count++;

    set_state(ECHOEAR_CONVERSATION_STATE_COMPLETE,
              ECHOEAR_CONVERSATION_ERROR_NONE);
    return true;
}

void echoear_conversation_fail(echoear_conversation_error_t error)
{
    if (error == ECHOEAR_CONVERSATION_ERROR_NONE)
        error = ECHOEAR_CONVERSATION_ERROR_INTERNAL;
    fail_active_turn(error);
}

void echoear_conversation_cancel(void)
{
    if (s_conversation.request_active || s_conversation.response_active)
        s_conversation.cancel_count++;

    clear_turn_buffers();

    if (!s_conversation.config.enabled) {
        set_state(ECHOEAR_CONVERSATION_STATE_IDLE,
                  ECHOEAR_CONVERSATION_ERROR_DISABLED);
    } else if (!s_conversation.gateway_ready) {
        set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
                  ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
    } else {
        set_state(ECHOEAR_CONVERSATION_STATE_READY,
                  ECHOEAR_CONVERSATION_ERROR_CANCELLED);
    }
}

void echoear_conversation_clear_completed(void)
{
    if (s_conversation.state != ECHOEAR_CONVERSATION_STATE_COMPLETE &&
        s_conversation.state != ECHOEAR_CONVERSATION_STATE_ERROR) return;

    clear_turn_buffers();

    if (!s_conversation.config.enabled) {
        set_state(ECHOEAR_CONVERSATION_STATE_IDLE,
                  ECHOEAR_CONVERSATION_ERROR_DISABLED);
    } else if (!s_conversation.gateway_ready) {
        set_state(ECHOEAR_CONVERSATION_STATE_OFFLINE,
                  ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE);
    } else {
        set_state(ECHOEAR_CONVERSATION_STATE_READY,
                  ECHOEAR_CONVERSATION_ERROR_NONE);
    }
}

void echoear_conversation_tick(uint32_t now_ms)
{
    s_conversation.now_ms = now_ms;
    if (s_conversation.deadline_ms == 0U ||
        now_ms < s_conversation.deadline_ms) return;

    if (s_conversation.state == ECHOEAR_CONVERSATION_STATE_REQUEST_PENDING) {
        fail_active_turn(ECHOEAR_CONVERSATION_ERROR_REQUEST_TIMEOUT);
    } else if (s_conversation.state == ECHOEAR_CONVERSATION_STATE_WAITING_RESPONSE ||
               s_conversation.state == ECHOEAR_CONVERSATION_STATE_STREAMING_RESPONSE) {
        fail_active_turn(ECHOEAR_CONVERSATION_ERROR_RESPONSE_TIMEOUT);
    }
}

bool echoear_conversation_is_ready(void)
{
    return s_conversation.config.enabled &&
           s_conversation.gateway_ready &&
           s_conversation.state == ECHOEAR_CONVERSATION_STATE_READY;
}

bool echoear_conversation_is_busy(void)
{
    return s_conversation.request_active ||
           s_conversation.response_active ||
           s_conversation.state == ECHOEAR_CONVERSATION_STATE_REQUEST_PENDING ||
           s_conversation.state == ECHOEAR_CONVERSATION_STATE_WAITING_RESPONSE ||
           s_conversation.state == ECHOEAR_CONVERSATION_STATE_STREAMING_RESPONSE;
}

const char *echoear_conversation_state_name(echoear_conversation_state_t state)
{
    switch (state) {
        case ECHOEAR_CONVERSATION_STATE_IDLE: return "idle";
        case ECHOEAR_CONVERSATION_STATE_OFFLINE: return "offline";
        case ECHOEAR_CONVERSATION_STATE_READY: return "ready";
        case ECHOEAR_CONVERSATION_STATE_REQUEST_PENDING: return "request_pending";
        case ECHOEAR_CONVERSATION_STATE_WAITING_RESPONSE: return "waiting_response";
        case ECHOEAR_CONVERSATION_STATE_STREAMING_RESPONSE: return "streaming_response";
        case ECHOEAR_CONVERSATION_STATE_COMPLETE: return "complete";
        case ECHOEAR_CONVERSATION_STATE_ERROR: return "error";
        default: return "unknown";
    }
}

const char *echoear_conversation_error_name(echoear_conversation_error_t error)
{
    switch (error) {
        case ECHOEAR_CONVERSATION_ERROR_NONE: return "none";
        case ECHOEAR_CONVERSATION_ERROR_INVALID_ARGUMENT: return "invalid_argument";
        case ECHOEAR_CONVERSATION_ERROR_DISABLED: return "disabled";
        case ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE: return "gateway_unavailable";
        case ECHOEAR_CONVERSATION_ERROR_SESSION_REQUIRED: return "session_required";
        case ECHOEAR_CONVERSATION_ERROR_EMPTY_INPUT: return "empty_input";
        case ECHOEAR_CONVERSATION_ERROR_BUSY: return "busy";
        case ECHOEAR_CONVERSATION_ERROR_REQUEST_TIMEOUT: return "request_timeout";
        case ECHOEAR_CONVERSATION_ERROR_RESPONSE_TIMEOUT: return "response_timeout";
        case ECHOEAR_CONVERSATION_ERROR_RESPONSE_TOO_LARGE: return "response_too_large";
        case ECHOEAR_CONVERSATION_ERROR_STALE_RESPONSE: return "stale_response";
        case ECHOEAR_CONVERSATION_ERROR_CANCELLED: return "cancelled";
        case ECHOEAR_CONVERSATION_ERROR_GATEWAY_ERROR: return "gateway_error";
        case ECHOEAR_CONVERSATION_ERROR_PROTOCOL: return "protocol";
        case ECHOEAR_CONVERSATION_ERROR_INTERNAL: return "internal";
        default: return "unknown";
    }
}
