#include "echoear_voice_conversation_bridge.h"

#include <string.h>

static echoear_voice_conversation_bridge_t s_bridge;
static echoear_voice_conversation_bridge_ops_t s_ops;

static bool text_present(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static bool copy_text(char *dst, size_t dst_size, const char *src)
{
    size_t length;

    if (dst == NULL || dst_size == 0U || src == NULL) {
        return false;
    }

    length = strlen(src);

    if (length >= dst_size) {
        return false;
    }

    memcpy(dst, src, length + 1U);
    return true;
}

static void clear_ids(void)
{
    s_bridge.voice_request_id[0] = '\0';
    s_bridge.conversation_request_id[0] = '\0';
}

static void bump_generation(void)
{
    s_bridge.generation++;
}

static void set_state(
    echoear_voice_conversation_state_t state,
    echoear_voice_conversation_error_t error
)
{
    if (s_bridge.state == state &&
        s_bridge.error == error) {
        return;
    }

    s_bridge.state = state;
    s_bridge.error = error;
    bump_generation();
}

static bool fail_bridge(
    echoear_voice_conversation_error_t error
)
{
    s_bridge.failures++;
    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_ERROR,
        error
    );
    return false;
}

static bool voice_request_matches(const char *request_id)
{
    if (!s_bridge.active ||
        !text_present(request_id) ||
        !text_present(s_bridge.voice_request_id)) {
        return false;
    }

    return strcmp(
        request_id,
        s_bridge.voice_request_id
    ) == 0;
}

static bool conversation_request_matches(const char *request_id)
{
    if (!s_bridge.active ||
        !text_present(request_id) ||
        !text_present(s_bridge.conversation_request_id)) {
        return false;
    }

    return strcmp(
        request_id,
        s_bridge.conversation_request_id
    ) == 0;
}

static bool ops_valid(
    const echoear_voice_conversation_bridge_ops_t *ops
)
{
    if (ops == NULL) {
        return false;
    }

    return
        ops->conversation_submit != NULL &&
        ops->conversation_cancel != NULL &&
        ops->voice_begin_output != NULL &&
        ops->voice_complete_turn != NULL &&
        ops->voice_interrupt != NULL;
}

void echoear_voice_conversation_bridge_init(void)
{
    memset(&s_bridge, 0, sizeof(s_bridge));
    memset(&s_ops, 0, sizeof(s_ops));

    s_bridge.enabled = true;
    s_bridge.state =
        ECHOEAR_VOICE_CONVERSATION_STATE_IDLE;
    s_bridge.error =
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE;
    s_bridge.generation = 1U;
}

void echoear_voice_conversation_bridge_reset(void)
{
    bool enabled = s_bridge.enabled;
    uint32_t generation = s_bridge.generation;

    memset(&s_bridge, 0, sizeof(s_bridge));

    s_bridge.enabled = enabled;
    s_bridge.configured = ops_valid(&s_ops);
    s_bridge.state =
        ECHOEAR_VOICE_CONVERSATION_STATE_IDLE;
    s_bridge.error =
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE;
    s_bridge.generation = generation + 1U;
}

echoear_voice_conversation_bridge_t *
echoear_voice_conversation_bridge_get(void)
{
    return &s_bridge;
}

bool echoear_voice_conversation_bridge_configure(
    const echoear_voice_conversation_bridge_ops_t *ops
)
{
    if (s_bridge.active) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_BUSY
        );
    }

    if (!ops_valid(ops)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_INVALID_ARGUMENT
        );
    }

    s_ops = *ops;
    s_bridge.configured = true;
    s_bridge.error =
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE;

    if (s_bridge.state ==
        ECHOEAR_VOICE_CONVERSATION_STATE_ERROR) {
        s_bridge.state =
            ECHOEAR_VOICE_CONVERSATION_STATE_IDLE;
    }

    bump_generation();
    return true;
}

void echoear_voice_conversation_bridge_set_enabled(
    bool enabled
)
{
    if (s_bridge.enabled == enabled) {
        return;
    }

    s_bridge.enabled = enabled;

    if (!enabled) {
        s_bridge.active = false;
        s_bridge.response_started = false;
        clear_ids();

        set_state(
            ECHOEAR_VOICE_CONVERSATION_STATE_IDLE,
            ECHOEAR_VOICE_CONVERSATION_ERROR_DISABLED
        );
        return;
    }

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_IDLE,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );
}

bool echoear_voice_conversation_bridge_start_turn(
    const char *voice_request_id
)
{
    if (!s_bridge.enabled) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_DISABLED
        );
    }

    if (!s_bridge.configured) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_NOT_CONFIGURED
        );
    }

    if (!text_present(voice_request_id)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_INVALID_ARGUMENT
        );
    }

    if (s_bridge.active) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_BUSY
        );
    }

    if (!copy_text(
            s_bridge.voice_request_id,
            sizeof(s_bridge.voice_request_id),
            voice_request_id)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_INVALID_ARGUMENT
        );
    }

    s_bridge.conversation_request_id[0] = '\0';
    s_bridge.active = true;
    s_bridge.response_started = false;
    s_bridge.turns_started++;

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_LISTENING,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );

    return true;
}

bool echoear_voice_conversation_bridge_submit_transcript(
    const char *voice_request_id,
    const char *transcript
)
{
    char conversation_request_id[
        ECHOEAR_VOICE_CONVERSATION_ID_MAX
    ];

    /*
     * Stale request protection:
     * mismatched IDs are ignored without changing state/error.
     */
    if (!voice_request_matches(voice_request_id)) {
        return false;
    }

    if (s_bridge.state !=
        ECHOEAR_VOICE_CONVERSATION_STATE_LISTENING) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_PROTOCOL
        );
    }

    if (!text_present(transcript)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_INVALID_ARGUMENT
        );
    }

    memset(
        conversation_request_id,
        0,
        sizeof(conversation_request_id)
    );

    if (!s_ops.conversation_submit(
            transcript,
            conversation_request_id,
            sizeof(conversation_request_id))) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_CONVERSATION_REJECTED
        );
    }

    if (!text_present(conversation_request_id)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_INTERNAL
        );
    }

    if (!copy_text(
            s_bridge.conversation_request_id,
            sizeof(s_bridge.conversation_request_id),
            conversation_request_id)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_INTERNAL
        );
    }

    s_bridge.transcripts_submitted++;

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_WAITING_RESPONSE,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );

    return true;
}

bool echoear_voice_conversation_bridge_response_begin(
    const char *conversation_request_id
)
{
    /*
     * Stale Conversation events are ignored without mutation.
     */
    if (!conversation_request_matches(
            conversation_request_id)) {
        return false;
    }

    if (s_bridge.state !=
        ECHOEAR_VOICE_CONVERSATION_STATE_WAITING_RESPONSE) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_PROTOCOL
        );
    }

    if (!s_ops.voice_begin_output(
            s_bridge.voice_request_id)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_VOICE_REJECTED
        );
    }

    s_bridge.response_started = true;
    s_bridge.responses_started++;

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_SPEAKING,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );

    return true;
}

bool echoear_voice_conversation_bridge_response_complete(
    const char *conversation_request_id
)
{
    if (!conversation_request_matches(
            conversation_request_id)) {
        return false;
    }

    if (s_bridge.state !=
            ECHOEAR_VOICE_CONVERSATION_STATE_SPEAKING ||
        !s_bridge.response_started) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_PROTOCOL
        );
    }

    if (!s_ops.voice_complete_turn(
            s_bridge.voice_request_id)) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_VOICE_REJECTED
        );
    }

    s_bridge.responses_completed++;
    s_bridge.active = false;
    s_bridge.response_started = false;
    clear_ids();

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_IDLE,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );

    return true;
}

bool echoear_voice_conversation_bridge_interrupt(
    const char *voice_request_id
)
{
    bool voice_ok;
    bool conversation_ok = true;

    if (!voice_request_matches(voice_request_id)) {
        return false;
    }

    if (s_bridge.state !=
            ECHOEAR_VOICE_CONVERSATION_STATE_WAITING_RESPONSE &&
        s_bridge.state !=
            ECHOEAR_VOICE_CONVERSATION_STATE_SPEAKING) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_PROTOCOL
        );
    }

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_INTERRUPTING,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );

    /*
     * Stop local audio/turn first for responsive barge-in.
     * Voice Core remains responsible for preserving its session.
     */
    voice_ok = s_ops.voice_interrupt();

    if (text_present(
            s_bridge.conversation_request_id)) {
        conversation_ok =
            s_ops.conversation_cancel(
                s_bridge.conversation_request_id
            );
    }

    s_bridge.interruptions++;

    s_bridge.active = false;
    s_bridge.response_started = false;
    clear_ids();

    if (!voice_ok) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_VOICE_REJECTED
        );
    }

    if (!conversation_ok) {
        return fail_bridge(
            ECHOEAR_VOICE_CONVERSATION_ERROR_CANCEL_FAILED
        );
    }

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_IDLE,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );

    return true;
}

void echoear_voice_conversation_bridge_voice_session_lost(void)
{
    if (!s_bridge.active) {
        return;
    }

    if (text_present(
            s_bridge.conversation_request_id) &&
        s_bridge.configured &&
        s_ops.conversation_cancel != NULL) {
        (void)s_ops.conversation_cancel(
            s_bridge.conversation_request_id
        );
    }

    s_bridge.active = false;
    s_bridge.response_started = false;
    clear_ids();

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_IDLE,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );
}

void echoear_voice_conversation_bridge_clear_error(void)
{
    if (s_bridge.state !=
        ECHOEAR_VOICE_CONVERSATION_STATE_ERROR) {
        return;
    }

    if (s_bridge.active) {
        return;
    }

    set_state(
        ECHOEAR_VOICE_CONVERSATION_STATE_IDLE,
        ECHOEAR_VOICE_CONVERSATION_ERROR_NONE
    );
}

bool echoear_voice_conversation_bridge_is_busy(void)
{
    return s_bridge.active;
}

const char *echoear_voice_conversation_bridge_state_name(
    echoear_voice_conversation_state_t state
)
{
    switch (state) {
        case ECHOEAR_VOICE_CONVERSATION_STATE_IDLE:
            return "idle";

        case ECHOEAR_VOICE_CONVERSATION_STATE_LISTENING:
            return "listening";

        case ECHOEAR_VOICE_CONVERSATION_STATE_WAITING_RESPONSE:
            return "waiting_response";

        case ECHOEAR_VOICE_CONVERSATION_STATE_SPEAKING:
            return "speaking";

        case ECHOEAR_VOICE_CONVERSATION_STATE_INTERRUPTING:
            return "interrupting";

        case ECHOEAR_VOICE_CONVERSATION_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}

const char *echoear_voice_conversation_bridge_error_name(
    echoear_voice_conversation_error_t error
)
{
    switch (error) {
        case ECHOEAR_VOICE_CONVERSATION_ERROR_NONE:
            return "none";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_DISABLED:
            return "disabled";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_NOT_CONFIGURED:
            return "not_configured";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_BUSY:
            return "busy";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_PROTOCOL:
            return "protocol";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_CONVERSATION_REJECTED:
            return "conversation_rejected";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_VOICE_REJECTED:
            return "voice_rejected";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_CANCEL_FAILED:
            return "cancel_failed";

        case ECHOEAR_VOICE_CONVERSATION_ERROR_INTERNAL:
            return "internal";

        default:
            return "unknown";
    }
}