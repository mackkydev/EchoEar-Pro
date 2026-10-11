#ifndef ECHOEAR_SPEECH_OUTPUT_H
#define ECHOEAR_SPEECH_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_SPEECH_REQUEST_ID_MAX 96
#define ECHOEAR_SPEECH_TEXT_MAX       1024

typedef enum {
    ECHOEAR_SPEECH_STATE_IDLE = 0,
    ECHOEAR_SPEECH_STATE_SYNTHESIZING,
    ECHOEAR_SPEECH_STATE_PLAYING,
    ECHOEAR_SPEECH_STATE_INTERRUPTING,
    ECHOEAR_SPEECH_STATE_ERROR
} echoear_speech_state_t;

typedef enum {
    ECHOEAR_SPEECH_ERROR_NONE = 0,
    ECHOEAR_SPEECH_ERROR_INVALID_ARGUMENT,
    ECHOEAR_SPEECH_ERROR_BUSY,
    ECHOEAR_SPEECH_ERROR_BACKEND,
    ECHOEAR_SPEECH_ERROR_PLAYBACK,
    ECHOEAR_SPEECH_ERROR_INTERNAL
} echoear_speech_error_t;

typedef struct {
    echoear_speech_state_t state;
    echoear_speech_error_t error;

    bool enabled;
    bool completion_pending;

    char request_id[ECHOEAR_SPEECH_REQUEST_ID_MAX];
    char text[ECHOEAR_SPEECH_TEXT_MAX];
    char completed_request_id[ECHOEAR_SPEECH_REQUEST_ID_MAX];

    uint32_t now_ms;
    uint32_t state_since_ms;

    uint32_t submitted_count;
    uint32_t started_count;
    uint32_t completed_count;
    uint32_t interrupted_count;
    uint32_t failure_count;
    uint32_t generation;
} echoear_speech_output_t;

void echoear_speech_output_init(void);
void echoear_speech_output_reset(void);

echoear_speech_output_t *echoear_speech_output_get(void);

bool echoear_speech_output_submit(
    const char *request_id,
    const char *text
);

bool echoear_speech_output_audio_started(
    const char *request_id
);

bool echoear_speech_output_audio_completed(
    const char *request_id
);

bool echoear_speech_output_interrupt(void);

void echoear_speech_output_fail(
    echoear_speech_error_t error
);

void echoear_speech_output_tick(
    uint32_t now_ms
);

void echoear_speech_output_ack_completion(void);

const char *echoear_speech_output_state_name(
    echoear_speech_state_t state
);

const char *echoear_speech_output_error_name(
    echoear_speech_error_t error
);

#ifdef __cplusplus
}
#endif

#endif
