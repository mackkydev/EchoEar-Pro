#include "echoear_voice_network_adapter.h"

#include "echoear_realtime_voice.h"
#include "echoear_runtime_network.h"
#include "echoear_runtime_reconnect.h"
#include "echoear_voice_conversation_bridge.h"

#include <string.h>

static echoear_voice_network_adapter_t s_adapter;

static bool runtime_ready(void)
{
    return echoear_runtime_network_is_ready() &&
           echoear_runtime_reconnect_get()->network_ready;
}

void echoear_voice_network_adapter_init(void)
{
    memset(&s_adapter, 0, sizeof(s_adapter));
    s_adapter.initialized = true;
    s_adapter.network_ready = runtime_ready();
    s_adapter.generation = 1U;
    echoear_realtime_voice_set_network_ready(s_adapter.network_ready);
}

void echoear_voice_network_adapter_poll(void)
{
    const bool ready = runtime_ready();

    if (!s_adapter.initialized || ready == s_adapter.network_ready) {
        return;
    }

    s_adapter.network_ready = ready;
    if (!ready) {
        /* Cancel the higher-level request before Voice Core clears its IDs. */
        echoear_voice_conversation_bridge_voice_session_lost();
        echoear_realtime_voice_set_network_ready(false);
        s_adapter.disconnects++;
    } else {
        echoear_realtime_voice_set_network_ready(true);
        s_adapter.restores++;
    }
    s_adapter.generation++;
}

echoear_voice_network_adapter_t *echoear_voice_network_adapter_get(void)
{
    return &s_adapter;
}
