#ifndef ECHOEAR_SEARCH_BRIDGE_H
#define ECHOEAR_SEARCH_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#include "echoear_live_search.h"
#include "echoear_tool_router.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECHOEAR_SEARCH_BRIDGE_STATE_IDLE = 0,
    ECHOEAR_SEARCH_BRIDGE_STATE_ROUTING,
    ECHOEAR_SEARCH_BRIDGE_STATE_COMPLETE,
    ECHOEAR_SEARCH_BRIDGE_STATE_ERROR
} echoear_search_bridge_state_t;

typedef struct {
    echoear_search_bridge_state_t state;

    bool active;

    char call_id[ECHOEAR_TOOL_CALL_ID_MAX];
    char search_request_id[ECHOEAR_SEARCH_REQUEST_ID_MAX];
    char query[ECHOEAR_SEARCH_QUERY_MAX];

    uint32_t routed_count;
    uint32_t completed_count;
    uint32_t failed_count;

    uint32_t generation;
} echoear_search_bridge_t;

void echoear_search_bridge_init(void);

void echoear_search_bridge_reset(void);

echoear_search_bridge_t *echoear_search_bridge_get(void);

void echoear_search_bridge_process(void);

const char *echoear_search_bridge_state_name(
    echoear_search_bridge_state_t state
);

#ifdef __cplusplus
}
#endif

#endif