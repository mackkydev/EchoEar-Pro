#ifndef ECHOEAR_XIAOZHI_AUDIO_ADAPTER_H
#define ECHOEAR_XIAOZHI_AUDIO_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_XIAOZHI_REQUEST_ID_MAX 96

typedef enum {
    ECHOEAR_XIAOZHI_AUDIO_STATE_IDLE = 0,
    ECHOEAR_XIAOZHI_AUDIO_STATE_WAITING_STREAM,
    ECHOEAR_XIAOZHI_AUDIO_STATE_STREAMING,
    ECHOEAR_XIAOZHI_AUDIO_STATE_DRAINING,
    ECHOEAR_XIAOZHI_AUDIO_STATE_INTERRUPTING,
    ECHOEAR_XIAOZHI_AUDIO_STATE_ERROR
} echoear_xiaozhi_audio_state_t;

typedef enum {
    ECHOEAR_XIAOZHI_AUDIO_ERROR_NONE = 0,
    ECHOEAR_XIAOZHI_AUDIO_ERROR_INVALID_ARGUMENT,
    ECHOEAR_XIAOZHI_AUDIO_ERROR_NOT_CONFIGURED,
    ECHOEAR_XIAOZHI_AUDIO_ERROR_BUSY,
    ECHOEAR_XIAOZHI_AUDIO_ERROR_PUSH_FAILED,
    ECHOEAR_XIAOZHI_AUDIO_ERROR_PLAYBACK_FAILED,
    ECHOEAR_XIAOZHI_AUDIO_ERROR_INTERNAL
} echoear_xiaozhi_audio_error_t;

typedef struct {
    bool initialized;
    bool configured;
    bool request_active;
    bool stream_started;
    bool stream_finished;

    echoear_xiaozhi_audio_state_t state;
    echoear_xiaozhi_audio_error_t error;

    char request_id[
        ECHOEAR_XIAOZHI_REQUEST_ID_MAX
    ];

    uint32_t packets_received;
    uint32_t packets_accepted;
    uint32_t packets_rejected;

    uint32_t streams_started;
    uint32_t streams_finished;
    uint32_t playbacks_drained;
    uint32_t interruptions;
    uint32_t failures;
    uint32_t generation;
} echoear_xiaozhi_audio_adapter_t;

void echoear_xiaozhi_audio_adapter_init(void);

echoear_xiaozhi_audio_adapter_t *
echoear_xiaozhi_audio_adapter_get(void);

bool echoear_xiaozhi_audio_adapter_begin(
    const char *request_id
);

bool echoear_xiaozhi_audio_adapter_push_opus(
    const char *request_id,
    const uint8_t *data,
    size_t size,
    int sample_rate,
    int frame_duration_ms
);

bool echoear_xiaozhi_audio_adapter_finish_stream(
    const char *request_id
);

bool echoear_xiaozhi_audio_adapter_interrupt(
    const char *request_id
);

void echoear_xiaozhi_audio_adapter_notify_playback_drained(
    const char *request_id
);

bool echoear_xiaozhi_audio_adapter_is_playback_idle(void);

const char *echoear_xiaozhi_audio_state_name(
    echoear_xiaozhi_audio_state_t state
);

const char *echoear_xiaozhi_audio_error_name(
    echoear_xiaozhi_audio_error_t error
);

#ifdef __cplusplus
}
#endif

#endif