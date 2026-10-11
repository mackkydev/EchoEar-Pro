#include "echoear_speech_ui_adapter.h"

#include "echoear_pro_ui.h"
#include "echoear_speech_output.h"

#include <stdio.h>
#include <string.h>

static echoear_speech_ui_adapter_t s_adapter;

static void apply_face(
    echoear_face_state_t face,
    const char *reason
)
{
    if (s_adapter.last_face_state == (int)face) {
        return;
    }

    echoear_pro_ui_set_state(face);

    s_adapter.last_face_state = (int)face;
    s_adapter.updates++;
    s_adapter.generation++;

    printf(
        "[SpeechUIAdapter] "
        "speech=%s face=%d reason=%s "
        "updates=%u generation=%u\n",
        echoear_speech_output_state_name(
            echoear_speech_output_get()->state
        ),
        (int)face,
        reason != NULL ? reason : "none",
        (unsigned)s_adapter.updates,
        (unsigned)s_adapter.generation
    );
}

void echoear_speech_ui_adapter_init(void)
{
    memset(&s_adapter, 0, sizeof(s_adapter));

    s_adapter.initialized = true;
    s_adapter.speech_ui_active = false;
    s_adapter.last_speech_state = -1;
    s_adapter.last_face_state = -1;
    s_adapter.generation = 1;
}

echoear_speech_ui_adapter_t *
echoear_speech_ui_adapter_get(void)
{
    return &s_adapter;
}

void echoear_speech_ui_adapter_poll(void)
{
    echoear_speech_output_t *speech =
        echoear_speech_output_get();

    if (!s_adapter.initialized ||
        speech == NULL) {
        return;
    }

    if (s_adapter.last_speech_state ==
        (int)speech->state) {
        return;
    }

    s_adapter.last_speech_state =
        (int)speech->state;

    switch (speech->state) {

        case ECHOEAR_SPEECH_STATE_SYNTHESIZING:

            s_adapter.speech_ui_active = true;

            apply_face(
                ECHOEAR_FACE_NORMAL_THINKING,
                "speech_synthesizing"
            );

            break;

        case ECHOEAR_SPEECH_STATE_PLAYING:

            s_adapter.speech_ui_active = true;

            apply_face(
                ECHOEAR_FACE_NORMAL_SPEAKING,
                "speech_playing"
            );

            break;

        case ECHOEAR_SPEECH_STATE_IDLE:

            /*
             * Only restore IDLE if SpeechOutput previously
             * owned the UI.
             *
             * This prevents normal idle polling from
             * overriding LISTENING / CAR / SYSTEM states.
             */
            if (s_adapter.speech_ui_active) {

                apply_face(
                    ECHOEAR_FACE_NORMAL_IDLE,
                    "speech_complete"
                );

                s_adapter.speech_ui_active = false;
            }

            break;

        case ECHOEAR_SPEECH_STATE_ERROR:

            s_adapter.speech_ui_active = true;

            apply_face(
                ECHOEAR_FACE_SYSTEM_ERROR,
                "speech_error"
            );

            break;

        case ECHOEAR_SPEECH_STATE_INTERRUPTING:

            /*
             * Keep the current face while interruption
             * is being processed.
             */
            break;

        default:
            break;
    }
}