#include "echoear_speech_backend.h"

#include "echoear_speech_output.h"

#include <string.h>

static echoear_speech_backend_t s_backend;

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

static bool request_matches(
    const char *request_id
)
{
    return request_id != NULL &&
           request_id[0] != '\0' &&
           s_backend.request_id[0] != '\0' &&
           strcmp(
               request_id,
               s_backend.request_id
           ) == 0;
}

static void set_state(
    echoear_speech_backend_state_t state,
    echoear_speech_backend_error_t error
)
{
    if (s_backend.state == state &&
        s_backend.error == error) {
        return;
    }

    s_backend.state = state;
    s_backend.error = error;
    s_backend.generation++;
}

void echoear_speech_backend_init(void)
{
    memset(&s_backend, 0, sizeof(s_backend));

    s_backend.backend =
        ECHOEAR_SPEECH_BACKEND_NONE;

    s_backend.state =
        ECHOEAR_SPEECH_BACKEND_STATE_IDLE;

    s_backend.error =
        ECHOEAR_SPEECH_BACKEND_ERROR_NONE;

    s_backend.generation = 1;
}

echoear_speech_backend_t *
echoear_speech_backend_get(void)
{
    return &s_backend;
}

bool echoear_speech_backend_configure(
    echoear_speech_backend_id_t backend,
    const echoear_speech_backend_ops_t *ops
)
{
    if (backend == ECHOEAR_SPEECH_BACKEND_NONE ||
        ops == NULL ||
        ops->submit == NULL) {

        s_backend.error =
            ECHOEAR_SPEECH_BACKEND_ERROR_INVALID_ARGUMENT;

        s_backend.failures++;
        s_backend.generation++;

        return false;
    }

    s_backend.backend = backend;
    s_backend.ops = *ops;
    s_backend.configured = true;
    s_backend.active = false;

    s_backend.request_id[0] = '\0';

    set_state(
        ECHOEAR_SPEECH_BACKEND_STATE_IDLE,
        ECHOEAR_SPEECH_BACKEND_ERROR_NONE
    );

    return true;
}

bool echoear_speech_backend_submit(
    const char *request_id,
    const char *text
)
{
    if (request_id == NULL ||
        request_id[0] == '\0' ||
        text == NULL ||
        text[0] == '\0') {

        s_backend.error =
            ECHOEAR_SPEECH_BACKEND_ERROR_INVALID_ARGUMENT;

        s_backend.failures++;
        s_backend.generation++;

        return false;
    }

    if (!s_backend.configured ||
        s_backend.ops.submit == NULL) {

        s_backend.error =
            ECHOEAR_SPEECH_BACKEND_ERROR_NOT_CONFIGURED;

        s_backend.failures++;
        s_backend.generation++;

        return false;
    }

    if (s_backend.active ||
        s_backend.state !=
            ECHOEAR_SPEECH_BACKEND_STATE_IDLE) {

        s_backend.error =
            ECHOEAR_SPEECH_BACKEND_ERROR_BUSY;

        s_backend.failures++;
        s_backend.generation++;

        return false;
    }

    copy_text(
        s_backend.request_id,
        sizeof(s_backend.request_id),
        request_id
    );

    s_backend.requests++;
    s_backend.active = true;

    set_state(
        ECHOEAR_SPEECH_BACKEND_STATE_REQUESTING,
        ECHOEAR_SPEECH_BACKEND_ERROR_NONE
    );

    if (!s_backend.ops.submit(
            request_id,
            text
        )) {

        s_backend.active = false;
        s_backend.request_id[0] = '\0';

        s_backend.failures++;

        set_state(
            ECHOEAR_SPEECH_BACKEND_STATE_ERROR,
            ECHOEAR_SPEECH_BACKEND_ERROR_REQUEST_FAILED
        );

        return false;
    }

    return true;
}

bool echoear_speech_backend_interrupt(void)
{
    if (!s_backend.active) {
        return false;
    }

    bool accepted = true;

    if (s_backend.ops.interrupt != NULL) {
        accepted = s_backend.ops.interrupt();
    }

    if (!accepted) {
        return false;
    }

    s_backend.interruptions++;
    s_backend.active = false;
    s_backend.request_id[0] = '\0';

    echoear_speech_output_interrupt();

    set_state(
        ECHOEAR_SPEECH_BACKEND_STATE_IDLE,
        ECHOEAR_SPEECH_BACKEND_ERROR_NONE
    );

    return true;
}

void echoear_speech_backend_stream_started(
    const char *request_id
)
{
    if (!request_matches(request_id) ||
        !s_backend.active) {
        return;
    }

    s_backend.streams_started++;

    set_state(
        ECHOEAR_SPEECH_BACKEND_STATE_STREAMING,
        ECHOEAR_SPEECH_BACKEND_ERROR_NONE
    );

    echoear_speech_output_audio_started(
        request_id
    );
}

void echoear_speech_backend_stream_finished(
    const char *request_id
)
{
    if (!request_matches(request_id) ||
        !s_backend.active) {
        return;
    }

    set_state(
        ECHOEAR_SPEECH_BACKEND_STATE_DRAINING,
        ECHOEAR_SPEECH_BACKEND_ERROR_NONE
    );
}

void echoear_speech_backend_playback_drained(
    const char *request_id
)
{
    if (!request_matches(request_id) ||
        !s_backend.active) {
        return;
    }

    if (s_backend.state !=
            ECHOEAR_SPEECH_BACKEND_STATE_STREAMING &&
        s_backend.state !=
            ECHOEAR_SPEECH_BACKEND_STATE_DRAINING) {
        return;
    }

    s_backend.streams_completed++;

    echoear_speech_output_audio_completed(
        request_id
    );

    s_backend.active = false;
    s_backend.request_id[0] = '\0';

    set_state(
        ECHOEAR_SPEECH_BACKEND_STATE_IDLE,
        ECHOEAR_SPEECH_BACKEND_ERROR_NONE
    );
}

void echoear_speech_backend_fail(
    echoear_speech_backend_error_t error
)
{
    s_backend.failures++;
    s_backend.active = false;
    s_backend.request_id[0] = '\0';

    set_state(
        ECHOEAR_SPEECH_BACKEND_STATE_ERROR,
        error
    );

    echoear_speech_output_fail(
        ECHOEAR_SPEECH_ERROR_BACKEND
    );
}

const char *echoear_speech_backend_name(
    echoear_speech_backend_id_t backend
)
{
    switch (backend) {

        case ECHOEAR_SPEECH_BACKEND_NONE:
            return "none";

        case ECHOEAR_SPEECH_BACKEND_MOCK:
            return "mock";

        case ECHOEAR_SPEECH_BACKEND_GATEWAY:
            return "gateway";

        case ECHOEAR_SPEECH_BACKEND_LOCAL:
            return "local";

        case ECHOEAR_SPEECH_BACKEND_NATIVE_AUDIO:
            return "native_audio";

        default:
            return "unknown";
    }
}

const char *echoear_speech_backend_state_name(
    echoear_speech_backend_state_t state
)
{
    switch (state) {

        case ECHOEAR_SPEECH_BACKEND_STATE_IDLE:
            return "idle";

        case ECHOEAR_SPEECH_BACKEND_STATE_REQUESTING:
            return "requesting";

        case ECHOEAR_SPEECH_BACKEND_STATE_STREAMING:
            return "streaming";

        case ECHOEAR_SPEECH_BACKEND_STATE_DRAINING:
            return "draining";

        case ECHOEAR_SPEECH_BACKEND_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}
