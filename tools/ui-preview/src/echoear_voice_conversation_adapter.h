#ifndef ECHOEAR_VOICE_CONVERSATION_ADAPTER_H
#define ECHOEAR_VOICE_CONVERSATION_ADAPTER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Connects Voice Conversation Bridge callbacks to the real
 * Conversation Engine and Realtime Voice cores.
 *
 * This adapter does not own:
 * - Wi-Fi
 * - provisioning
 * - AI providers
 * - gateway transport
 * - Tool Router
 */
bool echoear_voice_conversation_adapter_init(void);

void echoear_voice_conversation_adapter_reset_tracking(void);

const char *
echoear_voice_conversation_adapter_last_request_id(void);

#ifdef __cplusplus
}
#endif

#endif