#ifndef ECHOEAR_CONVERSATION_H
#define ECHOEAR_CONVERSATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_CONVERSATION_ID_MAX 64
#define ECHOEAR_CONVERSATION_REQUEST_ID_MAX 64
#define ECHOEAR_CONVERSATION_LOCALE_MAX 16
#define ECHOEAR_CONVERSATION_TEXT_MAX 768

typedef enum {
    ECHOEAR_CONVERSATION_STATE_IDLE = 0,
    ECHOEAR_CONVERSATION_STATE_OFFLINE,
    ECHOEAR_CONVERSATION_STATE_READY,
    ECHOEAR_CONVERSATION_STATE_REQUEST_PENDING,
    ECHOEAR_CONVERSATION_STATE_WAITING_RESPONSE,
    ECHOEAR_CONVERSATION_STATE_STREAMING_RESPONSE,
    ECHOEAR_CONVERSATION_STATE_COMPLETE,
    ECHOEAR_CONVERSATION_STATE_ERROR
} echoear_conversation_state_t;

typedef enum {
    ECHOEAR_CONVERSATION_ERROR_NONE = 0,
    ECHOEAR_CONVERSATION_ERROR_INVALID_ARGUMENT,
    ECHOEAR_CONVERSATION_ERROR_DISABLED,
    ECHOEAR_CONVERSATION_ERROR_GATEWAY_UNAVAILABLE,
    ECHOEAR_CONVERSATION_ERROR_SESSION_REQUIRED,
    ECHOEAR_CONVERSATION_ERROR_EMPTY_INPUT,
    ECHOEAR_CONVERSATION_ERROR_BUSY,
    ECHOEAR_CONVERSATION_ERROR_REQUEST_TIMEOUT,
    ECHOEAR_CONVERSATION_ERROR_RESPONSE_TIMEOUT,
    ECHOEAR_CONVERSATION_ERROR_RESPONSE_TOO_LARGE,
    ECHOEAR_CONVERSATION_ERROR_STALE_RESPONSE,
    ECHOEAR_CONVERSATION_ERROR_CANCELLED,
    ECHOEAR_CONVERSATION_ERROR_GATEWAY_ERROR,
    ECHOEAR_CONVERSATION_ERROR_PROTOCOL,
    ECHOEAR_CONVERSATION_ERROR_INTERNAL
} echoear_conversation_error_t;

typedef struct {
    bool enabled;
    uint32_t request_timeout_ms;
    uint32_t response_timeout_ms;
    char locale[ECHOEAR_CONVERSATION_LOCALE_MAX];
} echoear_conversation_config_t;

typedef struct {
    char conversation_id[ECHOEAR_CONVERSATION_ID_MAX];
    char request_id[ECHOEAR_CONVERSATION_REQUEST_ID_MAX];
    char locale[ECHOEAR_CONVERSATION_LOCALE_MAX];
    char text[ECHOEAR_CONVERSATION_TEXT_MAX];
} echoear_conversation_request_t;

typedef struct {
    echoear_conversation_state_t state;
    echoear_conversation_error_t error;
    echoear_conversation_config_t config;

    bool gateway_ready;
    bool session_active;
    bool request_active;
    bool response_active;

    char conversation_id[ECHOEAR_CONVERSATION_ID_MAX];
    char request_id[ECHOEAR_CONVERSATION_REQUEST_ID_MAX];
    char user_text[ECHOEAR_CONVERSATION_TEXT_MAX];
    char assistant_text[ECHOEAR_CONVERSATION_TEXT_MAX];
    size_t assistant_text_length;

    uint32_t turn_count;
    uint32_t request_count;
    uint32_t success_count;
    uint32_t failure_count;
    uint32_t cancel_count;

    uint32_t now_ms;
    uint32_t deadline_ms;
    uint32_t generation;
} echoear_conversation_t;

echoear_conversation_config_t echoear_conversation_default_config(void);

void echoear_conversation_init(void);
void echoear_conversation_reset(void);
echoear_conversation_t *echoear_conversation_get(void);

bool echoear_conversation_configure(const echoear_conversation_config_t *config);
void echoear_conversation_set_gateway_ready(bool ready);

bool echoear_conversation_start_session(const char *conversation_id);
void echoear_conversation_end_session(void);

bool echoear_conversation_submit_user_text(const char *request_id, const char *text);

bool echoear_conversation_get_pending_request(echoear_conversation_request_t *request);

bool echoear_conversation_mark_request_dispatched(const char *request_id);
bool echoear_conversation_response_begin(const char *request_id);
bool echoear_conversation_response_append(const char *request_id, const char *chunk);
bool echoear_conversation_response_complete(const char *request_id);

void echoear_conversation_fail(echoear_conversation_error_t error);
void echoear_conversation_cancel(void);
void echoear_conversation_clear_completed(void);
void echoear_conversation_tick(uint32_t now_ms);

bool echoear_conversation_is_ready(void);
bool echoear_conversation_is_busy(void);

const char *echoear_conversation_state_name(echoear_conversation_state_t state);
const char *echoear_conversation_error_name(echoear_conversation_error_t error);

#ifdef __cplusplus
}
#endif

#endif
