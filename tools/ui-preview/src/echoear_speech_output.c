#include "echoear_speech_output.h"

#include <string.h>

static echoear_speech_output_t s_speech;

static void copy_text(
    char *dst,
    size_t dst_size,
    const char *src
)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    size_t len = strlen(src);

    if (len >= dst_size) {
        len = dst_size - 1;
    }

    memcpy(dst, src, len);
    dst[len] = '\0';
}

static void set_state(
    echoear_speech_state_t state,
    echoear_speech_error_t error
)
{
    if (s_speech.state == state &&
        s_speech.error == error) {
        return;
    }

    s_speech.state = state;
    s_speech.error = error;
    s_speech.state_since_ms = s_speech.now_ms;
    s_speech.generation++;
}

static void clear_active(void)
{
    s_speech.request_id[0] = '\0';
    s_speech.text[0] = '\0';
}

void echoear_speech_output_init(void)
{
    memset(&s_speech, 0, sizeof(s_speech));

    s_speech.enabled = true;
    s_speech.state = ECHOEAR_SPEECH_STATE_IDLE;
    s_speech.error = ECHOEAR_SPEECH_ERROR_NONE;
    s_speech.generation = 1;
}

void echoear_speech_output_reset(void)
{
    echoear_speech_output_init();
}

echoear_speech_output_t *echoear_speech_output_get(void)
{
    return &s_speech;
}

bool echoear_speech_output_submit(
    const char *request_id,
    const char *text
)
{
    if (request_id == NULL ||
        request_id[0] == '\0' ||
        text == NULL ||
        text[0] == '\0') {

        s_speech.error =
            ECHOEAR_SPEECH_ERROR_INVALID_ARGUMENT;

        s_speech.failure_count++;
        s_speech.generation++;

        return false;
    }

    if (!s_speech.enabled ||
        s_speech.state != ECHOEAR_SPEECH_STATE_IDLE ||
        s_speech.completion_pending) {

        s_speech.error = ECHOEAR_SPEECH_ERROR_BUSY;
        s_speech.failure_count++;
        s_speech.generation++;

        return false;
    }

    copy_text(
        s_speech.request_id,
        sizeof(s_speech.request_id),
        request_id
    );

    copy_text(
        s_speech.text,
        sizeof(s_speech.text),
        text
    );

    s_speech.submitted_count++;

    set_state(
        ECHOEAR_SPEECH_STATE_SYNTHESIZING,
        ECHOEAR_SPEECH_ERROR_NONE
    );

    return true;
}

bool echoear_speech_output_audio_started(
    const char *request_id
)
{
    if (request_id == NULL ||
        strcmp(request_id, s_speech.request_id) != 0 ||
        s_speech.state !=
            ECHOEAR_SPEECH_STATE_SYNTHESIZING) {

        return false;
    }

    s_speech.started_count++;

    set_state(
        ECHOEAR_SPEECH_STATE_PLAYING,
        ECHOEAR_SPEECH_ERROR_NONE
    );

    return true;
}

bool echoear_speech_output_audio_completed(
    const char *request_id
)
{
    if (request_id == NULL ||
        strcmp(request_id, s_speech.request_id) != 0 ||
        s_speech.state !=
            ECHOEAR_SPEECH_STATE_PLAYING) {

        return false;
    }

    copy_text(
        s_speech.completed_request_id,
        sizeof(s_speech.completed_request_id),
        s_speech.request_id
    );

    s_speech.completed_count++;
    s_speech.completion_pending = true;

    clear_active();

    set_state(
        ECHOEAR_SPEECH_STATE_IDLE,
        ECHOEAR_SPEECH_ERROR_NONE
    );

    return true;
}

bool echoear_speech_output_interrupt(void)
{
    if (s_speech.state !=
            ECHOEAR_SPEECH_STATE_SYNTHESIZING &&
        s_speech.state !=
            ECHOEAR_SPEECH_STATE_PLAYING) {

        return false;
    }

    set_state(
        ECHOEAR_SPEECH_STATE_INTERRUPTING,
        ECHOEAR_SPEECH_ERROR_NONE
    );

    s_speech.interrupted_count++;

    clear_active();

    set_state(
        ECHOEAR_SPEECH_STATE_IDLE,
        ECHOEAR_SPEECH_ERROR_NONE
    );

    return true;
}

void echoear_speech_output_fail(
    echoear_speech_error_t error
)
{
    s_speech.failure_count++;

    clear_active();

    set_state(
        ECHOEAR_SPEECH_STATE_ERROR,
        error
    );
}

void echoear_speech_output_tick(
    uint32_t now_ms
)
{
    s_speech.now_ms = now_ms;
}

void echoear_speech_output_ack_completion(void)
{
    s_speech.completion_pending = false;
    s_speech.completed_request_id[0] = '\0';
    s_speech.generation++;
}

const char *echoear_speech_output_state_name(
    echoear_speech_state_t state
)
{
    switch (state) {
        case ECHOEAR_SPEECH_STATE_IDLE:
            return "idle";

        case ECHOEAR_SPEECH_STATE_SYNTHESIZING:
            return "synthesizing";

        case ECHOEAR_SPEECH_STATE_PLAYING:
            return "playing";

        case ECHOEAR_SPEECH_STATE_INTERRUPTING:
            return "interrupting";

        case ECHOEAR_SPEECH_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}

const char *echoear_speech_output_error_name(
    echoear_speech_error_t error
)
{
    switch (error) {
        case ECHOEAR_SPEECH_ERROR_NONE:
            return "none";

        case ECHOEAR_SPEECH_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case ECHOEAR_SPEECH_ERROR_BUSY:
            return "busy";

        case ECHOEAR_SPEECH_ERROR_BACKEND:
            return "backend";

        case ECHOEAR_SPEECH_ERROR_PLAYBACK:
            return "playback";

        case ECHOEAR_SPEECH_ERROR_INTERNAL:
            return "internal";

        default:
            return "unknown";
    }
}