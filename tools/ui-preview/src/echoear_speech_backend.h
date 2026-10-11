#ifndef ECHOEAR_SPEECH_BACKEND_H
#define ECHOEAR_SPEECH_BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_SPEECH_BACKEND_REQUEST_ID_MAX 96

typedef enum {
    ECHOEAR_SPEECH_BACKEND_NONE = 0,
    ECHOEAR_SPEECH_BACKEND_MOCK,
    ECHOEAR_SPEECH_BACKEND_GATEWAY,
    ECHOEAR_SPEECH_BACKEND_LOCAL,
    ECHOEAR_SPEECH_BACKEND_NATIVE_AUDIO
} echoear_speech_backend_id_t;

typedef enum {
    ECHOEAR_SPEECH_BACKEND_STATE_IDLE = 0,
    ECHOEAR_SPEECH_BACKEND_STATE_REQUESTING,
    ECHOEAR_SPEECH_BACKEND_STATE_STREAMING,
    ECHOEAR_SPEECH_BACKEND_STATE_DRAINING,
    ECHOEAR_SPEECH_BACKEND_STATE_ERROR
} echoear_speech_backend_state_t;

typedef enum {
    ECHOEAR_SPEECH_BACKEND_ERROR_NONE = 0,
    ECHOEAR_SPEECH_BACKEND_ERROR_INVALID_ARGUMENT,
    ECHOEAR_SPEECH_BACKEND_ERROR_NOT_CONFIGURED,
    ECHOEAR_SPEECH_BACKEND_ERROR_BUSY,
    ECHOEAR_SPEECH_BACKEND_ERROR_REQUEST_FAILED,
    ECHOEAR_SPEECH_BACKEND_ERROR_STREAM_FAILED,
    ECHOEAR_SPEECH_BACKEND_ERROR_PLAYBACK_FAILED
} echoear_speech_backend_error_t;

typedef struct {
    bool (*submit)(
        const char *request_id,
        const char *text
    );

    bool (*interrupt)(void);

    bool (*is_playback_idle)(void);
} echoear_speech_backend_ops_t;

typedef struct {
    echoear_speech_backend_id_t backend;
    echoear_speech_backend_state_t state;
    echoear_speech_backend_error_t error;

    bool configured;
    bool active;

    char request_id[
        ECHOEAR_SPEECH_BACKEND_REQUEST_ID_MAX
    ];

    uint32_t requests;
    uint32_t streams_started;
    uint32_t streams_completed;
    uint32_t interruptions;
    uint32_t failures;
    uint32_t generation;

    echoear_speech_backend_ops_t ops;
} echoear_speech_backend_t;

void echoear_speech_backend_init(void);

echoear_speech_backend_t *
echoear_speech_backend_get(void);

bool echoear_speech_backend_configure(
    echoear_speech_backend_id_t backend,
    const echoear_speech_backend_ops_t *ops
);

bool echoear_speech_backend_submit(
    const char *request_id,
    const char *text
);

bool echoear_speech_backend_interrupt(void);

void echoear_speech_backend_stream_started(
    const char *request_id
);

void echoear_speech_backend_stream_finished(
    const char *request_id
);

void echoear_speech_backend_playback_drained(
    const char *request_id
);

void echoear_speech_backend_fail(
    echoear_speech_backend_error_t error
);

const char *echoear_speech_backend_name(
    echoear_speech_backend_id_t backend
);

const char *echoear_speech_backend_state_name(
    echoear_speech_backend_state_t state
);

#ifdef __cplusplus
}
#endif

#endif