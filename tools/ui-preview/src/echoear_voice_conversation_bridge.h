#ifndef ECHOEAR_VOICE_CONVERSATION_BRIDGE_H
#define ECHOEAR_VOICE_CONVERSATION_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_VOICE_CONVERSATION_ID_MAX 64U

typedef enum {
    ECHOEAR_VOICE_CONVERSATION_STATE_IDLE = 0,
    ECHOEAR_VOICE_CONVERSATION_STATE_LISTENING,
    ECHOEAR_VOICE_CONVERSATION_STATE_WAITING_RESPONSE,
    ECHOEAR_VOICE_CONVERSATION_STATE_SPEAKING,
    ECHOEAR_VOICE_CONVERSATION_STATE_INTERRUPTING,
    ECHOEAR_VOICE_CONVERSATION_STATE_ERROR
} echoear_voice_conversation_state_t;

typedef enum {
    ECHOEAR_VOICE_CONVERSATION_ERROR_NONE = 0,
    ECHOEAR_VOICE_CONVERSATION_ERROR_DISABLED,
    ECHOEAR_VOICE_CONVERSATION_ERROR_NOT_CONFIGURED,
    ECHOEAR_VOICE_CONVERSATION_ERROR_BUSY,
    ECHOEAR_VOICE_CONVERSATION_ERROR_INVALID_ARGUMENT,
    ECHOEAR_VOICE_CONVERSATION_ERROR_PROTOCOL,
    ECHOEAR_VOICE_CONVERSATION_ERROR_CONVERSATION_REJECTED,
    ECHOEAR_VOICE_CONVERSATION_ERROR_VOICE_REJECTED,
    ECHOEAR_VOICE_CONVERSATION_ERROR_CANCEL_FAILED,
    ECHOEAR_VOICE_CONVERSATION_ERROR_INTERNAL
} echoear_voice_conversation_error_t;

/*
 * Adapter boundary.
 *
 * The bridge intentionally does not depend directly on provider,
 * gateway, Wi-Fi, provisioning or Tool Router implementations.
 *
 * conversation_submit:
 *   Submit final user transcript to Conversation Engine.
 *   Adapter writes the resulting Conversation request ID into
 *   conversation_request_id_out.
 *
 * conversation_cancel:
 *   Cancel only the active Conversation request.
 *
 * voice_begin_output:
 *   Transition the matching Realtime Voice turn into output.
 *
 * voice_complete_turn:
 *   Complete the matching Realtime Voice turn.
 *
 * voice_interrupt:
 *   Interrupt the current Realtime Voice turn while preserving
 *   the voice session.
 */
typedef struct {
    bool (*conversation_submit)(
        const char *transcript,
        char *conversation_request_id_out,
        size_t conversation_request_id_out_size
    );

    bool (*conversation_cancel)(
        const char *conversation_request_id
    );

    bool (*voice_begin_output)(
        const char *voice_request_id
    );

    bool (*voice_complete_turn)(
        const char *voice_request_id
    );

    bool (*voice_interrupt)(void);
} echoear_voice_conversation_bridge_ops_t;

typedef struct {
    echoear_voice_conversation_state_t state;
    echoear_voice_conversation_error_t error;

    bool enabled;
    bool configured;
    bool active;
    bool response_started;

    char voice_request_id[ECHOEAR_VOICE_CONVERSATION_ID_MAX];
    char conversation_request_id[ECHOEAR_VOICE_CONVERSATION_ID_MAX];

    uint32_t turns_started;
    uint32_t transcripts_submitted;
    uint32_t responses_started;
    uint32_t responses_completed;
    uint32_t interruptions;
    uint32_t failures;
    uint32_t generation;
} echoear_voice_conversation_bridge_t;

void echoear_voice_conversation_bridge_init(void);

void echoear_voice_conversation_bridge_reset(void);

echoear_voice_conversation_bridge_t *
echoear_voice_conversation_bridge_get(void);

bool echoear_voice_conversation_bridge_configure(
    const echoear_voice_conversation_bridge_ops_t *ops
);

void echoear_voice_conversation_bridge_set_enabled(
    bool enabled
);

bool echoear_voice_conversation_bridge_start_turn(
    const char *voice_request_id
);

bool echoear_voice_conversation_bridge_submit_transcript(
    const char *voice_request_id,
    const char *transcript
);

bool echoear_voice_conversation_bridge_response_begin(
    const char *conversation_request_id
);

bool echoear_voice_conversation_bridge_response_complete(
    const char *conversation_request_id
);

bool echoear_voice_conversation_bridge_interrupt(
    const char *voice_request_id
);

void echoear_voice_conversation_bridge_voice_session_lost(void);

void echoear_voice_conversation_bridge_clear_error(void);

bool echoear_voice_conversation_bridge_is_busy(void);

const char *echoear_voice_conversation_bridge_state_name(
    echoear_voice_conversation_state_t state
);

const char *echoear_voice_conversation_bridge_error_name(
    echoear_voice_conversation_error_t error
);

#ifdef __cplusplus
}
#endif

#endif