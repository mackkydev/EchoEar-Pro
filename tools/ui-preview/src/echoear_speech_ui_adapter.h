#ifndef ECHOEAR_SPEECH_UI_ADAPTER_H
#define ECHOEAR_SPEECH_UI_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool initialized;
    bool speech_ui_active;

    int last_speech_state;
    int last_face_state;

    uint32_t updates;
    uint32_t generation;
} echoear_speech_ui_adapter_t;

void echoear_speech_ui_adapter_init(void);

void echoear_speech_ui_adapter_poll(void);

echoear_speech_ui_adapter_t *
echoear_speech_ui_adapter_get(void);

#ifdef __cplusplus
}
#endif

#endif