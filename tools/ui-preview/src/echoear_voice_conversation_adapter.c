#include "echoear_voice_conversation_adapter.h"

#include "echoear_conversation.h"
#include "echoear_realtime_voice.h"
#include "echoear_voice_conversation_bridge.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t s_request_counter = 0U;

static char s_last_conversation_request_id[
    ECHOEAR_VOICE_CONVERSATION_ID_MAX
];

static bool text_present(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static bool adapter_conversation_submit(
    const char *transcript,
    char *conversation_request_id_out,
    size_t conversation_request_id_out_size
)
{
    char request_id[ECHOEAR_VOICE_CONVERSATION_ID_MAX];
    int written;
    size_t request_id_length;

    if (!text_present(transcript) ||
        conversation_request_id_out == NULL ||
        conversation_request_id_out_size == 0U) {
        return false;
    }

    s_request_counter++;

    if (s_request_counter == 0U) {
        s_request_counter = 1U;
    }

    written = snprintf(
        request_id,
        sizeof(request_id),
        "voice-conv-%08lu",
        (unsigned long)s_request_counter
    );

    if (written <= 0 ||
        (size_t)written >= sizeof(request_id)) {
        return false;
    }

    request_id_length = strlen(request_id);

    /*
     * Validate destination before touching Conversation Core.
     */
    if (request_id_length + 1U >
        conversation_request_id_out_size) {
        return false;
    }

    if (!echoear_conversation_submit_user_text(
            request_id,
            transcript)) {
        return false;
    }

    memcpy(
        conversation_request_id_out,
        request_id,
        request_id_length + 1U
    );

    memcpy(
        s_last_conversation_request_id,
        request_id,
        request_id_length + 1U
    );

    return true;
}

static bool adapter_conversation_cancel(
    const char *conversation_request_id
)
{
    if (!text_present(conversation_request_id) ||
        !text_present(s_last_conversation_request_id)) {
        return false;
    }

    /*
     * Conversation Core currently exposes cancel(void), so the
     * adapter verifies the request ID before invoking cancel.
     */
    if (strcmp(
            conversation_request_id,
            s_last_conversation_request_id) != 0) {
        return false;
    }

    echoear_conversation_cancel();

    s_last_conversation_request_id[0] = '\0';

    return true;
}

static bool adapter_voice_begin_output(
    const char *voice_request_id
)
{
    if (!text_present(voice_request_id)) {
        return false;
    }

    return echoear_realtime_voice_begin_output(
        voice_request_id
    );
}

static bool adapter_voice_complete_turn(
    const char *voice_request_id
)
{
    bool accepted;

    if (!text_present(voice_request_id)) {
        return false;
    }

    accepted =
        echoear_realtime_voice_complete_turn(
            voice_request_id
        );

    if (accepted) {
        s_last_conversation_request_id[0] = '\0';
    }

    return accepted;
}

static bool adapter_voice_interrupt(void)
{
    return echoear_realtime_voice_interrupt();
}

bool echoear_voice_conversation_adapter_init(void)
{
    echoear_voice_conversation_bridge_ops_t ops;

    memset(&ops, 0, sizeof(ops));

    ops.conversation_submit =
        adapter_conversation_submit;

    ops.conversation_cancel =
        adapter_conversation_cancel;

    ops.voice_begin_output =
        adapter_voice_begin_output;

    ops.voice_complete_turn =
        adapter_voice_complete_turn;

    ops.voice_interrupt =
        adapter_voice_interrupt;

    return echoear_voice_conversation_bridge_configure(
        &ops
    );
}

void echoear_voice_conversation_adapter_reset_tracking(void)
{
    s_request_counter = 0U;
    s_last_conversation_request_id[0] = '\0';
}

const char *
echoear_voice_conversation_adapter_last_request_id(void)
{
    if (!text_present(
            s_last_conversation_request_id)) {
        return NULL;
    }

    return s_last_conversation_request_id;
}