#include "echoear_realtime_voice.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static echoear_realtime_voice_t s_voice;

static size_t bounded_length(
    const char *text,
    size_t maximum
)
{
    size_t length = 0U;

    if (text == NULL) {
        return 0U;
    }

    while (length < maximum &&
           text[length] != '\0') {
        length++;
    }

    return length;
}

static bool valid_id(
    const char *id,
    size_t maximum
)
{
    size_t length =
        bounded_length(id, maximum);

    return length > 0U &&
           length < maximum;
}

static void copy_text(
    char *destination,
    size_t destination_size,
    const char *source
)
{
    if (destination == NULL ||
        destination_size == 0U) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    snprintf(
        destination,
        destination_size,
        "%s",
        source
    );

    destination[destination_size - 1U] = '\0';
}

static bool same_id(
    const char *left,
    const char *right
)
{
    if (left == NULL ||
        right == NULL) {
        return false;
    }

    return strcmp(left, right) == 0;
}

static bool deadline_reached(
    uint32_t now_ms,
    uint32_t deadline_ms
)
{
    if (deadline_ms == 0U) {
        return false;
    }

    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static uint32_t make_deadline(
    uint32_t now_ms,
    uint32_t timeout_ms
)
{
    uint32_t deadline;

    if (timeout_ms == 0U) {
        return 0U;
    }

    deadline = now_ms + timeout_ms;

    /*
     * Zero is reserved to mean "no deadline".
     */
    if (deadline == 0U) {
        deadline = 1U;
    }

    return deadline;
}

static void set_state(
    echoear_voice_state_t state,
    echoear_voice_error_t error
)
{
    if (s_voice.state == state &&
        s_voice.error == error) {
        return;
    }

    s_voice.state = state;
    s_voice.error = error;
    s_voice.generation++;
}

static void clear_turn(void)
{
    s_voice.request_id[0] = '\0';

    s_voice.input_active = false;
    s_voice.output_active = false;

    s_voice.input_sequence = 0U;
    s_voice.output_sequence = 0U;

    s_voice.deadline_ms = 0U;
}

static void clear_session(void)
{
    clear_turn();

    s_voice.session_id[0] = '\0';
    s_voice.session_active = false;
}

static bool availability_ready(void)
{
    return s_voice.config.enabled &&
           s_voice.network_ready &&
           s_voice.gateway_ready;
}

static void apply_idle_availability(void)
{
    if (!s_voice.config.enabled) {
        set_state(
            ECHOEAR_VOICE_STATE_OFFLINE,
            ECHOEAR_VOICE_ERROR_DISABLED
        );

        return;
    }

    if (!s_voice.network_ready) {
        set_state(
            ECHOEAR_VOICE_STATE_OFFLINE,
            ECHOEAR_VOICE_ERROR_NETWORK_UNAVAILABLE
        );

        return;
    }

    if (!s_voice.gateway_ready) {
        set_state(
            ECHOEAR_VOICE_STATE_OFFLINE,
            ECHOEAR_VOICE_ERROR_GATEWAY_UNAVAILABLE
        );

        return;
    }

    set_state(
        ECHOEAR_VOICE_STATE_READY,
        ECHOEAR_VOICE_ERROR_NONE
    );
}

static void fail_transport(
    echoear_voice_error_t error
)
{
    bool had_active_work =
        s_voice.session_active ||
        s_voice.state == ECHOEAR_VOICE_STATE_CONNECTING ||
        s_voice.state == ECHOEAR_VOICE_STATE_LISTENING ||
        s_voice.state == ECHOEAR_VOICE_STATE_WAITING_RESPONSE ||
        s_voice.state == ECHOEAR_VOICE_STATE_SPEAKING ||
        s_voice.state == ECHOEAR_VOICE_STATE_INTERRUPTING;

    clear_session();

    if (had_active_work) {
        s_voice.failure_count++;

        set_state(
            ECHOEAR_VOICE_STATE_ERROR,
            error
        );

        return;
    }

    set_state(
        ECHOEAR_VOICE_STATE_OFFLINE,
        error
    );
}

static bool validate_config(
    const echoear_voice_config_t *config
)
{
    if (config == NULL) {
        return false;
    }

    if (config->codec != ECHOEAR_VOICE_CODEC_PCM16 &&
        config->codec != ECHOEAR_VOICE_CODEC_OPUS) {
        return false;
    }

    if (config->sample_rate_hz < 8000U ||
        config->sample_rate_hz > 48000U) {
        return false;
    }

    if (config->channels == 0U ||
        config->channels > 2U) {
        return false;
    }

    if (config->connect_timeout_ms == 0U ||
        config->response_timeout_ms == 0U) {
        return false;
    }

    return true;
}

void echoear_realtime_voice_default_config(
    echoear_voice_config_t *config
)
{
    if (config == NULL) {
        return;
    }

    memset(
        config,
        0,
        sizeof(*config)
    );

    config->enabled = true;

    config->codec =
        ECHOEAR_VOICE_CODEC_PCM16;

    config->sample_rate_hz = 16000U;
    config->channels = 1U;

    config->connect_timeout_ms = 10000U;
    config->response_timeout_ms = 15000U;
}

void echoear_realtime_voice_init(void)
{
    memset(
        &s_voice,
        0,
        sizeof(s_voice)
    );

    echoear_realtime_voice_default_config(
        &s_voice.config
    );

    s_voice.state =
        ECHOEAR_VOICE_STATE_OFFLINE;

    s_voice.error =
        ECHOEAR_VOICE_ERROR_NETWORK_UNAVAILABLE;

    s_voice.generation = 1U;
}

void echoear_realtime_voice_reset(void)
{
    echoear_realtime_voice_init();
}

echoear_realtime_voice_t *
echoear_realtime_voice_get(void)
{
    return &s_voice;
}

bool echoear_realtime_voice_configure(
    const echoear_voice_config_t *config
)
{
    if (!validate_config(config)) {
        return false;
    }

    if (echoear_realtime_voice_is_busy() ||
        s_voice.session_active) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_BUSY;

        s_voice.generation++;

        return false;
    }

    s_voice.config = *config;

    apply_idle_availability();

    return true;
}

void echoear_realtime_voice_set_network_ready(
    bool ready
)
{
    if (s_voice.network_ready == ready) {
        return;
    }

    s_voice.network_ready = ready;
    s_voice.generation++;

    if (!ready) {
        fail_transport(
            ECHOEAR_VOICE_ERROR_NETWORK_UNAVAILABLE
        );

        return;
    }

    if (!s_voice.session_active &&
        !echoear_realtime_voice_is_busy()) {
        apply_idle_availability();
    }
}

void echoear_realtime_voice_set_gateway_ready(
    bool ready
)
{
    if (s_voice.gateway_ready == ready) {
        return;
    }

    s_voice.gateway_ready = ready;
    s_voice.generation++;

    if (!ready) {
        fail_transport(
            ECHOEAR_VOICE_ERROR_GATEWAY_UNAVAILABLE
        );

        return;
    }

    if (!s_voice.session_active &&
        !echoear_realtime_voice_is_busy()) {
        apply_idle_availability();
    }
}

bool echoear_realtime_voice_start_session(
    const char *session_id
)
{
    if (!valid_id(
            session_id,
            ECHOEAR_VOICE_SESSION_ID_MAX)) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_PROTOCOL;

        s_voice.generation++;

        return false;
    }

    if (!availability_ready()) {
        if (!s_voice.config.enabled) {
            s_voice.error =
                ECHOEAR_VOICE_ERROR_DISABLED;
        } else if (!s_voice.network_ready) {
            s_voice.error =
                ECHOEAR_VOICE_ERROR_NETWORK_UNAVAILABLE;
        } else {
            s_voice.error =
                ECHOEAR_VOICE_ERROR_GATEWAY_UNAVAILABLE;
        }

        s_voice.generation++;

        return false;
    }

    if (echoear_realtime_voice_is_busy() ||
        s_voice.session_active) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_BUSY;

        s_voice.generation++;

        return false;
    }

    copy_text(
        s_voice.session_id,
        sizeof(s_voice.session_id),
        session_id
    );

    clear_turn();

    s_voice.deadline_ms =
        make_deadline(
            s_voice.now_ms,
            s_voice.config.connect_timeout_ms
        );

    s_voice.sessions_started++;

    set_state(
        ECHOEAR_VOICE_STATE_CONNECTING,
        ECHOEAR_VOICE_ERROR_NONE
    );

    return true;
}

bool echoear_realtime_voice_session_connected(
    const char *session_id
)
{
    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_CONNECTING) {
        return false;
    }

    /*
     * Stale session acknowledgements must not mutate state.
     */
    if (!same_id(
            session_id,
            s_voice.session_id)) {
        return false;
    }

    s_voice.session_active = true;
    s_voice.deadline_ms = 0U;

    set_state(
        ECHOEAR_VOICE_STATE_READY,
        ECHOEAR_VOICE_ERROR_NONE
    );

    return true;
}

bool echoear_realtime_voice_start_turn(
    const char *request_id
)
{
    if (!valid_id(
            request_id,
            ECHOEAR_VOICE_REQUEST_ID_MAX)) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_PROTOCOL;

        s_voice.generation++;

        return false;
    }

    if (!availability_ready()) {
        return false;
    }

    if (!s_voice.session_active) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_SESSION_UNAVAILABLE;

        s_voice.generation++;

        return false;
    }

    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_READY) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_BUSY;

        s_voice.generation++;

        return false;
    }

    copy_text(
        s_voice.request_id,
        sizeof(s_voice.request_id),
        request_id
    );

    s_voice.input_sequence = 0U;
    s_voice.output_sequence = 0U;

    s_voice.input_active = true;
    s_voice.output_active = false;

    s_voice.deadline_ms = 0U;

    s_voice.turns_started++;

    set_state(
        ECHOEAR_VOICE_STATE_LISTENING,
        ECHOEAR_VOICE_ERROR_NONE
    );

    return true;
}

bool echoear_realtime_voice_push_input(
    const echoear_voice_audio_chunk_t *chunk
)
{
    if (chunk == NULL) {
        return false;
    }

    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_LISTENING ||
        !s_voice.input_active) {
        return false;
    }

    if (chunk->length == 0U ||
        chunk->length >
            ECHOEAR_VOICE_AUDIO_CHUNK_MAX) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_AUDIO_TOO_LARGE;

        s_voice.generation++;

        return false;
    }

    /*
     * Deterministic ordering.
     * Duplicate, skipped, or stale chunks are rejected.
     */
    if (chunk->sequence !=
        s_voice.input_sequence) {
        return false;
    }

    s_voice.input_sequence++;

    if (chunk->end_of_turn) {
        s_voice.input_active = false;

        s_voice.deadline_ms =
            make_deadline(
                s_voice.now_ms,
                s_voice.config.response_timeout_ms
            );

        set_state(
            ECHOEAR_VOICE_STATE_WAITING_RESPONSE,
            ECHOEAR_VOICE_ERROR_NONE
        );
    }

    return true;
}

bool echoear_realtime_voice_end_input(
    const char *request_id
)
{
    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_LISTENING ||
        !s_voice.input_active) {
        return false;
    }

    /*
     * Stale request IDs are ignored without mutation.
     */
    if (!same_id(
            request_id,
            s_voice.request_id)) {
        return false;
    }

    s_voice.input_active = false;

    s_voice.deadline_ms =
        make_deadline(
            s_voice.now_ms,
            s_voice.config.response_timeout_ms
        );

    set_state(
        ECHOEAR_VOICE_STATE_WAITING_RESPONSE,
        ECHOEAR_VOICE_ERROR_NONE
    );

    return true;
}

bool echoear_realtime_voice_begin_output(
    const char *request_id
)
{
    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_WAITING_RESPONSE) {
        return false;
    }

    if (!same_id(
            request_id,
            s_voice.request_id)) {
        return false;
    }

    s_voice.output_active = true;
    s_voice.output_sequence = 0U;

    /*
     * First response arrived, so response timeout is satisfied.
     */
    s_voice.deadline_ms = 0U;

    set_state(
        ECHOEAR_VOICE_STATE_SPEAKING,
        ECHOEAR_VOICE_ERROR_NONE
    );

    return true;
}

bool echoear_realtime_voice_receive_output(
    const char *request_id,
    const echoear_voice_audio_chunk_t *chunk
)
{
    if (chunk == NULL) {
        return false;
    }

    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_SPEAKING ||
        !s_voice.output_active) {
        return false;
    }

    if (!same_id(
            request_id,
            s_voice.request_id)) {
        return false;
    }

    if (chunk->length == 0U ||
        chunk->length >
            ECHOEAR_VOICE_AUDIO_CHUNK_MAX) {
        s_voice.error =
            ECHOEAR_VOICE_ERROR_AUDIO_TOO_LARGE;

        s_voice.generation++;

        return false;
    }

    if (chunk->sequence !=
        s_voice.output_sequence) {
        return false;
    }

    s_voice.output_sequence++;

    if (chunk->end_of_turn) {
        s_voice.output_active = false;
    }

    return true;
}

bool echoear_realtime_voice_complete_turn(
    const char *request_id
)
{
    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_WAITING_RESPONSE &&
        s_voice.state !=
            ECHOEAR_VOICE_STATE_SPEAKING) {
        return false;
    }

    if (!same_id(
            request_id,
            s_voice.request_id)) {
        return false;
    }

    s_voice.turns_completed++;

    clear_turn();

    if (availability_ready() &&
        s_voice.session_active) {
        set_state(
            ECHOEAR_VOICE_STATE_READY,
            ECHOEAR_VOICE_ERROR_NONE
        );
    } else {
        apply_idle_availability();
    }

    return true;
}

bool echoear_realtime_voice_interrupt(void)
{
    if (s_voice.state !=
            ECHOEAR_VOICE_STATE_SPEAKING &&
        s_voice.state !=
            ECHOEAR_VOICE_STATE_WAITING_RESPONSE) {
        return false;
    }

    s_voice.input_active = false;
    s_voice.output_active = false;
    s_voice.deadline_ms = 0U;

    s_voice.interruptions++;

    set_state(
        ECHOEAR_VOICE_STATE_INTERRUPTING,
        ECHOEAR_VOICE_ERROR_NONE
    );

    return true;
}

void echoear_realtime_voice_disconnect(void)
{
    clear_session();

    apply_idle_availability();
}

void echoear_realtime_voice_tick(
    uint32_t now_ms
)
{
    s_voice.now_ms = now_ms;

    /*
     * INTERRUPTING is intentionally observable for one cycle.
     * The transport/backend can cancel its response, then the
     * next tick releases the core for another turn.
     */
    if (s_voice.state ==
            ECHOEAR_VOICE_STATE_INTERRUPTING) {

        clear_turn();

        if (availability_ready() &&
            s_voice.session_active) {
            set_state(
                ECHOEAR_VOICE_STATE_READY,
                ECHOEAR_VOICE_ERROR_NONE
            );
        } else {
            apply_idle_availability();
        }

        return;
    }

    if (!deadline_reached(
            s_voice.now_ms,
            s_voice.deadline_ms)) {
        return;
    }

    if (s_voice.state ==
            ECHOEAR_VOICE_STATE_CONNECTING ||
        s_voice.state ==
            ECHOEAR_VOICE_STATE_WAITING_RESPONSE) {

        clear_session();

        s_voice.failure_count++;

        set_state(
            ECHOEAR_VOICE_STATE_ERROR,
            ECHOEAR_VOICE_ERROR_TIMEOUT
        );
    }
}

bool echoear_realtime_voice_is_ready(void)
{
    return availability_ready() &&
           s_voice.state ==
               ECHOEAR_VOICE_STATE_READY;
}

bool echoear_realtime_voice_is_busy(void)
{
    switch (s_voice.state) {
        case ECHOEAR_VOICE_STATE_CONNECTING:
        case ECHOEAR_VOICE_STATE_LISTENING:
        case ECHOEAR_VOICE_STATE_WAITING_RESPONSE:
        case ECHOEAR_VOICE_STATE_SPEAKING:
        case ECHOEAR_VOICE_STATE_INTERRUPTING:
            return true;

        default:
            return false;
    }
}

const char *echoear_voice_state_name(
    echoear_voice_state_t state
)
{
    switch (state) {
        case ECHOEAR_VOICE_STATE_OFFLINE:
            return "offline";

        case ECHOEAR_VOICE_STATE_READY:
            return "ready";

        case ECHOEAR_VOICE_STATE_CONNECTING:
            return "connecting";

        case ECHOEAR_VOICE_STATE_LISTENING:
            return "listening";

        case ECHOEAR_VOICE_STATE_WAITING_RESPONSE:
            return "waiting_response";

        case ECHOEAR_VOICE_STATE_SPEAKING:
            return "speaking";

        case ECHOEAR_VOICE_STATE_INTERRUPTING:
            return "interrupting";

        case ECHOEAR_VOICE_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}

const char *echoear_voice_error_name(
    echoear_voice_error_t error
)
{
    switch (error) {
        case ECHOEAR_VOICE_ERROR_NONE:
            return "none";

        case ECHOEAR_VOICE_ERROR_DISABLED:
            return "disabled";

        case ECHOEAR_VOICE_ERROR_NETWORK_UNAVAILABLE:
            return "network_unavailable";

        case ECHOEAR_VOICE_ERROR_GATEWAY_UNAVAILABLE:
            return "gateway_unavailable";

        case ECHOEAR_VOICE_ERROR_SESSION_UNAVAILABLE:
            return "session_unavailable";

        case ECHOEAR_VOICE_ERROR_BUSY:
            return "busy";

        case ECHOEAR_VOICE_ERROR_AUDIO_TOO_LARGE:
            return "audio_too_large";

        case ECHOEAR_VOICE_ERROR_TIMEOUT:
            return "timeout";

        case ECHOEAR_VOICE_ERROR_TRANSPORT:
            return "transport";

        case ECHOEAR_VOICE_ERROR_PROTOCOL:
            return "protocol";

        case ECHOEAR_VOICE_ERROR_CANCELLED:
            return "cancelled";

        case ECHOEAR_VOICE_ERROR_INTERNAL:
            return "internal";

        default:
            return "unknown";
    }
}