#ifndef ECHOEAR_REALTIME_VOICE_H
#define ECHOEAR_REALTIME_VOICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_VOICE_SESSION_ID_MAX      64
#define ECHOEAR_VOICE_REQUEST_ID_MAX      64
#define ECHOEAR_VOICE_AUDIO_CHUNK_MAX     2048

typedef enum {
    ECHOEAR_VOICE_STATE_OFFLINE = 0,
    ECHOEAR_VOICE_STATE_READY,
    ECHOEAR_VOICE_STATE_CONNECTING,
    ECHOEAR_VOICE_STATE_LISTENING,
    ECHOEAR_VOICE_STATE_WAITING_RESPONSE,
    ECHOEAR_VOICE_STATE_SPEAKING,
    ECHOEAR_VOICE_STATE_INTERRUPTING,
    ECHOEAR_VOICE_STATE_ERROR
} echoear_voice_state_t;

typedef enum {
    ECHOEAR_VOICE_ERROR_NONE = 0,
    ECHOEAR_VOICE_ERROR_DISABLED,
    ECHOEAR_VOICE_ERROR_NETWORK_UNAVAILABLE,
    ECHOEAR_VOICE_ERROR_GATEWAY_UNAVAILABLE,
    ECHOEAR_VOICE_ERROR_SESSION_UNAVAILABLE,
    ECHOEAR_VOICE_ERROR_BUSY,
    ECHOEAR_VOICE_ERROR_AUDIO_TOO_LARGE,
    ECHOEAR_VOICE_ERROR_TIMEOUT,
    ECHOEAR_VOICE_ERROR_TRANSPORT,
    ECHOEAR_VOICE_ERROR_PROTOCOL,
    ECHOEAR_VOICE_ERROR_CANCELLED,
    ECHOEAR_VOICE_ERROR_INTERNAL
} echoear_voice_error_t;

typedef enum {
    ECHOEAR_VOICE_CODEC_PCM16 = 0,
    ECHOEAR_VOICE_CODEC_OPUS
} echoear_voice_codec_t;

typedef struct {
    bool enabled;

    echoear_voice_codec_t codec;

    uint32_t sample_rate_hz;
    uint8_t channels;

    uint32_t connect_timeout_ms;
    uint32_t response_timeout_ms;
} echoear_voice_config_t;

typedef struct {
    uint8_t data[ECHOEAR_VOICE_AUDIO_CHUNK_MAX];
    size_t length;

    uint32_t sequence;
    bool end_of_turn;
} echoear_voice_audio_chunk_t;

typedef struct {
    echoear_voice_state_t state;
    echoear_voice_error_t error;

    echoear_voice_config_t config;

    bool network_ready;
    bool gateway_ready;

    bool session_active;
    bool input_active;
    bool output_active;

    char session_id[ECHOEAR_VOICE_SESSION_ID_MAX];
    char request_id[ECHOEAR_VOICE_REQUEST_ID_MAX];

    uint32_t input_sequence;
    uint32_t output_sequence;

    uint32_t sessions_started;
    uint32_t turns_started;
    uint32_t turns_completed;
    uint32_t interruptions;
    uint32_t failure_count;

    uint32_t deadline_ms;
    uint32_t now_ms;
    uint32_t generation;
} echoear_realtime_voice_t;

void echoear_realtime_voice_init(void);

void echoear_realtime_voice_reset(void);

echoear_realtime_voice_t *echoear_realtime_voice_get(void);

void echoear_realtime_voice_default_config(
    echoear_voice_config_t *config
);

bool echoear_realtime_voice_configure(
    const echoear_voice_config_t *config
);

void echoear_realtime_voice_set_network_ready(
    bool ready
);

void echoear_realtime_voice_set_gateway_ready(
    bool ready
);

bool echoear_realtime_voice_start_session(
    const char *session_id
);

bool echoear_realtime_voice_session_connected(
    const char *session_id
);

bool echoear_realtime_voice_start_turn(
    const char *request_id
);

bool echoear_realtime_voice_push_input(
    const echoear_voice_audio_chunk_t *chunk
);

bool echoear_realtime_voice_end_input(
    const char *request_id
);

bool echoear_realtime_voice_begin_output(
    const char *request_id
);

bool echoear_realtime_voice_receive_output(
    const char *request_id,
    const echoear_voice_audio_chunk_t *chunk
);

bool echoear_realtime_voice_complete_turn(
    const char *request_id
);

bool echoear_realtime_voice_interrupt(void);

void echoear_realtime_voice_disconnect(void);

void echoear_realtime_voice_tick(
    uint32_t now_ms
);

bool echoear_realtime_voice_is_ready(void);

bool echoear_realtime_voice_is_busy(void);

const char *echoear_voice_state_name(
    echoear_voice_state_t state
);

const char *echoear_voice_error_name(
    echoear_voice_error_t error
);

#ifdef __cplusplus
}
#endif

#endif