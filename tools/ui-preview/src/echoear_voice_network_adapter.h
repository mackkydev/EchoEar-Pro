#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool initialized;
    bool network_ready;
    uint32_t disconnects;
    uint32_t restores;
    uint32_t generation;
} echoear_voice_network_adapter_t;

void echoear_voice_network_adapter_init(void);
void echoear_voice_network_adapter_poll(void);
echoear_voice_network_adapter_t *echoear_voice_network_adapter_get(void);
