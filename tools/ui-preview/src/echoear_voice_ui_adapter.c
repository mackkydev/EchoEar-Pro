#include "echoear_voice_ui_adapter.h"

#include "echoear_app_state.h"
#include "echoear_realtime_voice.h"

#include <string.h>

static echoear_voice_ui_adapter_t s_adapter;

void echoear_voice_ui_adapter_init(void)
{
    memset(&s_adapter, 0, sizeof(s_adapter));
    s_adapter.last_voice_state = -1;
    s_adapter.generation = 1U;
}

void echoear_voice_ui_adapter_poll(void)
{
    const echoear_voice_state_t state = echoear_realtime_voice_get()->state;
    echoear_interaction_state_t interaction;

    if (s_adapter.last_voice_state == (int)state) {
        return;
    }

    switch (state) {
        case ECHOEAR_VOICE_STATE_LISTENING:
            interaction = ECHOEAR_INTERACTION_LISTENING;
            break;
        case ECHOEAR_VOICE_STATE_CONNECTING:
        case ECHOEAR_VOICE_STATE_WAITING_RESPONSE:
            interaction = ECHOEAR_INTERACTION_THINKING;
            break;
        case ECHOEAR_VOICE_STATE_SPEAKING:
            interaction = ECHOEAR_INTERACTION_SPEAKING;
            break;
        case ECHOEAR_VOICE_STATE_ERROR:
            interaction = ECHOEAR_INTERACTION_CONFUSED;
            break;
        case ECHOEAR_VOICE_STATE_INTERRUPTING:
            /* Keep speaking visible until decoder/transport cancellation has
             * completed; the next READY tick releases the UI to idle. */
            interaction = ECHOEAR_INTERACTION_SPEAKING;
            break;
        case ECHOEAR_VOICE_STATE_OFFLINE:
        case ECHOEAR_VOICE_STATE_READY:
        default:
            interaction = ECHOEAR_INTERACTION_IDLE;
            break;
    }

    echoear_app_state_set_interaction_state(interaction);
    s_adapter.last_voice_state = (int)state;
    s_adapter.updates++;
    s_adapter.generation++;
}

echoear_voice_ui_adapter_t *echoear_voice_ui_adapter_get(void)
{
    return &s_adapter;
}
