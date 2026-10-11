#pragma once

#include <stdint.h>

typedef struct {
    int last_voice_state;
    uint32_t updates;
    uint32_t generation;
} echoear_voice_ui_adapter_t;

void echoear_voice_ui_adapter_init(void);
void echoear_voice_ui_adapter_poll(void);
echoear_voice_ui_adapter_t *echoear_voice_ui_adapter_get(void);
