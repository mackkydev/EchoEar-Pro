#include "echoear_xiaozhi_audio_adapter.h"

#include "echoear_speech_backend.h"

#include <cstring>
#include <cstdio>

#if defined(ECHOEAR_XIAOZHI_RUNTIME)
extern bool EchoEarAudioRuntimePushOpus(
    const uint8_t *data,
    size_t size,
    int sample_rate,
    int frame_duration_ms
);

extern bool EchoEarAudioRuntimeIsPlaybackIdle(void);
extern void EchoEarAudioRuntimeResetDecoder(void);
#endif

static echoear_xiaozhi_audio_adapter_t s_adapter;

/*
 * 4D.7D integration boundary
 *
 * This file intentionally does NOT include XiaoZhi headers yet.
 *
 * The adapter state machine is implemented here first.
 * XiaoZhi AudioService binding will be added in the ESP-IDF integration
 * layer after we verify the exact project/component include boundary.
 */

static bool request_matches(const char *request_id)
{
    if (request_id == nullptr ||
        request_id[0] == '\0' ||
        !s_adapter.request_active) {
        return false;
    }

    return std::strncmp(
        request_id,
        s_adapter.request_id,
        ECHOEAR_XIAOZHI_REQUEST_ID_MAX
    ) == 0;
}

static void clear_request(void)
{
    s_adapter.request_active = false;
    s_adapter.stream_started = false;
    s_adapter.stream_finished = false;
    s_adapter.request_id[0] = '\0';
}

void echoear_xiaozhi_audio_adapter_init(void)
{
    std::memset(&s_adapter, 0, sizeof(s_adapter));

    s_adapter.initialized = true;
    s_adapter.configured = true;
    s_adapter.state = ECHOEAR_XIAOZHI_AUDIO_STATE_IDLE;
    s_adapter.error = ECHOEAR_XIAOZHI_AUDIO_ERROR_NONE;
    s_adapter.generation = 1;

    std::printf(
        "[XiaoZhiAudioAdapterInit] configured=1\n"
    );
}

echoear_xiaozhi_audio_adapter_t *
echoear_xiaozhi_audio_adapter_get(void)
{
    return &s_adapter;
}

bool echoear_xiaozhi_audio_adapter_begin(
    const char *request_id
)
{
    if (!s_adapter.initialized ||
        !s_adapter.configured ||
        request_id == nullptr ||
        request_id[0] == '\0') {

        s_adapter.error =
            ECHOEAR_XIAOZHI_AUDIO_ERROR_INVALID_ARGUMENT;

        return false;
    }

    if (s_adapter.request_active ||
        s_adapter.state != ECHOEAR_XIAOZHI_AUDIO_STATE_IDLE) {

        s_adapter.error =
            ECHOEAR_XIAOZHI_AUDIO_ERROR_BUSY;

        return false;
    }

    std::strncpy(
        s_adapter.request_id,
        request_id,
        ECHOEAR_XIAOZHI_REQUEST_ID_MAX - 1
    );

    s_adapter.request_id[
        ECHOEAR_XIAOZHI_REQUEST_ID_MAX - 1
    ] = '\0';

    s_adapter.request_active = true;
    s_adapter.stream_started = false;
    s_adapter.stream_finished = false;

    s_adapter.state =
        ECHOEAR_XIAOZHI_AUDIO_STATE_WAITING_STREAM;

    s_adapter.error =
        ECHOEAR_XIAOZHI_AUDIO_ERROR_NONE;

    s_adapter.generation++;

    std::printf(
        "[XiaoZhiAudioBegin] request_id=%s\n",
        s_adapter.request_id
    );

    return true;
}

bool echoear_xiaozhi_audio_adapter_push_opus(
    const char *request_id,
    const uint8_t *data,
    size_t size,
    int sample_rate,
    int frame_duration_ms
)
{
    s_adapter.packets_received++;

    if (!request_matches(request_id) ||
        data == nullptr ||
        size == 0 ||
        sample_rate <= 0 ||
        frame_duration_ms <= 0) {

        s_adapter.packets_rejected++;
        s_adapter.error =
            ECHOEAR_XIAOZHI_AUDIO_ERROR_INVALID_ARGUMENT;

        return false;
    }

    if (s_adapter.stream_finished ||
        (s_adapter.state !=
             ECHOEAR_XIAOZHI_AUDIO_STATE_WAITING_STREAM &&
         s_adapter.state !=
             ECHOEAR_XIAOZHI_AUDIO_STATE_STREAMING)) {

        s_adapter.packets_rejected++;
        s_adapter.error =
            ECHOEAR_XIAOZHI_AUDIO_ERROR_BUSY;

        return false;
    }

    bool runtime_accepted = true;

#if defined(ECHOEAR_XIAOZHI_RUNTIME)
    runtime_accepted = EchoEarAudioRuntimePushOpus(
        data,
        size,
        sample_rate,
        frame_duration_ms
    );
#endif

    if (!runtime_accepted) {
        s_adapter.packets_rejected++;
        s_adapter.failures++;
        s_adapter.state = ECHOEAR_XIAOZHI_AUDIO_STATE_ERROR;
        s_adapter.error = ECHOEAR_XIAOZHI_AUDIO_ERROR_PUSH_FAILED;
        s_adapter.generation++;

        std::printf(
            "[XiaoZhiAudioPushFailed] request_id=%s size=%zu\n",
            request_id,
            size
        );

        return false;
    }

    s_adapter.packets_accepted++;

    if (!s_adapter.stream_started) {
        s_adapter.stream_started = true;

        s_adapter.state =
            ECHOEAR_XIAOZHI_AUDIO_STATE_STREAMING;

        s_adapter.streams_started++;
        s_adapter.generation++;

        echoear_speech_backend_stream_started(
            request_id
        );

        std::printf(
            "[XiaoZhiAudioStreamStart] "
            "request_id=%s sample_rate=%d frame_ms=%d\n",
            request_id,
            sample_rate,
            frame_duration_ms
        );
    }

    return true;
}

bool echoear_xiaozhi_audio_adapter_finish_stream(
    const char *request_id
)
{
    if (!request_matches(request_id) ||
        !s_adapter.stream_started ||
        s_adapter.stream_finished) {

        s_adapter.error =
            ECHOEAR_XIAOZHI_AUDIO_ERROR_INVALID_ARGUMENT;

        return false;
    }

    s_adapter.stream_finished = true;

    s_adapter.state =
        ECHOEAR_XIAOZHI_AUDIO_STATE_DRAINING;

    s_adapter.streams_finished++;
    s_adapter.generation++;

    echoear_speech_backend_stream_finished(
        request_id
    );

    std::printf(
        "[XiaoZhiAudioStreamFinish] request_id=%s\n",
        request_id
    );

    return true;
}

void echoear_xiaozhi_audio_adapter_notify_playback_drained(
    const char *request_id
)
{
    if (!request_matches(request_id)) {
        return;
    }

    /*
     * Ignore an early drained notification while the TTS stream
     * is still producing packets.
     */
    if (!s_adapter.stream_finished ||
        s_adapter.state !=
            ECHOEAR_XIAOZHI_AUDIO_STATE_DRAINING) {
        return;
    }

    char completed_request_id[
        ECHOEAR_XIAOZHI_REQUEST_ID_MAX
    ];

    std::strncpy(
        completed_request_id,
        s_adapter.request_id,
        sizeof(completed_request_id) - 1
    );

    completed_request_id[
        sizeof(completed_request_id) - 1
    ] = '\0';

    s_adapter.playbacks_drained++;
    s_adapter.state =
        ECHOEAR_XIAOZHI_AUDIO_STATE_IDLE;

    s_adapter.error =
        ECHOEAR_XIAOZHI_AUDIO_ERROR_NONE;

    s_adapter.generation++;

    clear_request();

    echoear_speech_backend_playback_drained(
        completed_request_id
    );

    std::printf(
        "[XiaoZhiAudioPlaybackDrained] request_id=%s\n",
        completed_request_id
    );
}

bool echoear_xiaozhi_audio_adapter_interrupt(
    const char *request_id
)
{
    if (!request_matches(request_id)) {
        return false;
    }

    s_adapter.state =
        ECHOEAR_XIAOZHI_AUDIO_STATE_INTERRUPTING;

    s_adapter.interruptions++;
    s_adapter.generation++;

#if defined(ECHOEAR_XIAOZHI_RUNTIME)
    EchoEarAudioRuntimeResetDecoder();
#endif

    clear_request();
    s_adapter.state = ECHOEAR_XIAOZHI_AUDIO_STATE_IDLE;
    s_adapter.error = ECHOEAR_XIAOZHI_AUDIO_ERROR_NONE;
    s_adapter.generation++;

    std::printf(
        "[XiaoZhiAudioInterrupt] request_id=%s\n",
        request_id
    );

    return true;
}

bool echoear_xiaozhi_audio_adapter_is_playback_idle(void)
{
#if defined(ECHOEAR_XIAOZHI_RUNTIME)
    if (s_adapter.request_active &&
        !EchoEarAudioRuntimeIsPlaybackIdle()) {
        return false;
    }
#endif

    return
        s_adapter.state ==
            ECHOEAR_XIAOZHI_AUDIO_STATE_IDLE &&
        !s_adapter.request_active;
}

const char *echoear_xiaozhi_audio_state_name(
    echoear_xiaozhi_audio_state_t state
)
{
    switch (state) {
        case ECHOEAR_XIAOZHI_AUDIO_STATE_IDLE:
            return "idle";

        case ECHOEAR_XIAOZHI_AUDIO_STATE_WAITING_STREAM:
            return "waiting_stream";

        case ECHOEAR_XIAOZHI_AUDIO_STATE_STREAMING:
            return "streaming";

        case ECHOEAR_XIAOZHI_AUDIO_STATE_DRAINING:
            return "draining";

        case ECHOEAR_XIAOZHI_AUDIO_STATE_INTERRUPTING:
            return "interrupting";

        case ECHOEAR_XIAOZHI_AUDIO_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}

const char *echoear_xiaozhi_audio_error_name(
    echoear_xiaozhi_audio_error_t error
)
{
    switch (error) {
        case ECHOEAR_XIAOZHI_AUDIO_ERROR_NONE:
            return "none";

        case ECHOEAR_XIAOZHI_AUDIO_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case ECHOEAR_XIAOZHI_AUDIO_ERROR_NOT_CONFIGURED:
            return "not_configured";

        case ECHOEAR_XIAOZHI_AUDIO_ERROR_BUSY:
            return "busy";

        case ECHOEAR_XIAOZHI_AUDIO_ERROR_PUSH_FAILED:
            return "push_failed";

        case ECHOEAR_XIAOZHI_AUDIO_ERROR_PLAYBACK_FAILED:
            return "playback_failed";

        case ECHOEAR_XIAOZHI_AUDIO_ERROR_INTERNAL:
            return "internal";

        default:
            return "unknown";
    }
}
