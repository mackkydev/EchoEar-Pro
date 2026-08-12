#include "echoear_search_bridge.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static echoear_search_bridge_t s_bridge;

static void bridge_set_state(
    echoear_search_bridge_state_t state,
    bool active
)
{
    if (s_bridge.state == state &&
        s_bridge.active == active) {
        return;
    }

    s_bridge.state = state;
    s_bridge.active = active;
    s_bridge.generation++;
}

static void clear_active_request(void)
{
    s_bridge.call_id[0] = '\0';
    s_bridge.search_request_id[0] = '\0';
    s_bridge.query[0] = '\0';
    s_bridge.active = false;
}

static void copy_text(
    char *destination,
    size_t destination_size,
    const char *source
)
{
    if (destination == NULL ||
        destination_size == 0U) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    snprintf(
        destination,
        destination_size,
        "%s",
        source
    );

    destination[destination_size - 1U] = '\0';
}

static bool copy_trimmed_range(
    const char *start,
    size_t length,
    char *output,
    size_t output_size
)
{
    size_t begin = 0U;
    size_t end = length;
    size_t copy_length;

    if (start == NULL ||
        output == NULL ||
        output_size == 0U) {
        return false;
    }

    while (begin < end &&
           isspace((unsigned char)start[begin])) {
        begin++;
    }

    while (end > begin &&
           isspace((unsigned char)start[end - 1U])) {
        end--;
    }

    copy_length = end - begin;

    if (copy_length == 0U ||
        copy_length >= output_size) {
        return false;
    }

    memcpy(
        output,
        start + begin,
        copy_length
    );

    output[copy_length] = '\0';

    return true;
}

static bool extract_json_string(
    const char *start,
    char *output,
    size_t output_size
)
{
    size_t output_length = 0U;
    const char *cursor = start;

    if (cursor == NULL ||
        output == NULL ||
        output_size == 0U ||
        *cursor != '"') {
        return false;
    }

    cursor++;

    while (*cursor != '\0' &&
           *cursor != '"') {
        char value = *cursor;

        if (value == '\\' &&
            cursor[1] != '\0') {
            cursor++;

            switch (*cursor) {
                case 'n':
                    value = '\n';
                    break;

                case 'r':
                    value = '\r';
                    break;

                case 't':
                    value = '\t';
                    break;

                case '"':
                    value = '"';
                    break;

                case '\\':
                    value = '\\';
                    break;

                default:
                    value = *cursor;
                    break;
            }
        }

        if (output_length + 1U >= output_size) {
            return false;
        }

        output[output_length++] = value;
        cursor++;
    }

    if (*cursor != '"' ||
        output_length == 0U) {
        return false;
    }

    output[output_length] = '\0';

    return true;
}

static bool extract_query(
    const char *arguments,
    char *query,
    size_t query_size
)
{
    const char *cursor;
    const char *query_key;
    const char *colon;

    if (arguments == NULL ||
        query == NULL ||
        query_size == 0U) {
        return false;
    }

    cursor = arguments;

    while (*cursor != '\0' &&
           isspace((unsigned char)*cursor)) {
        cursor++;
    }

    if (*cursor == '\0') {
        return false;
    }

    /*
     * Also accept a plain string for internal callers.
     */
    if (*cursor != '{') {
        return copy_trimmed_range(
            cursor,
            strlen(cursor),
            query,
            query_size
        );
    }

    query_key = strstr(cursor, "\"query\"");

    if (query_key == NULL) {
        return false;
    }

    colon = strchr(query_key + 7, ':');

    if (colon == NULL) {
        return false;
    }

    cursor = colon + 1;

    while (*cursor != '\0' &&
           isspace((unsigned char)*cursor)) {
        cursor++;
    }

    if (*cursor == '"') {
        return extract_json_string(
            cursor,
            query,
            query_size
        );
    }

    {
        const char *end = cursor;

        while (*end != '\0' &&
               *end != ',' &&
               *end != '}') {
            end++;
        }

        return copy_trimmed_range(
            cursor,
            (size_t)(end - cursor),
            query,
            query_size
        );
    }
}

static void build_tool_result(
    const echoear_live_search_t *search,
    char *output,
    size_t output_size
)
{
    if (output == NULL ||
        output_size == 0U) {
        return;
    }

    if (search == NULL ||
        search->result_count == 0U) {
        snprintf(
            output,
            output_size,
            "search_complete results=0"
        );

        output[output_size - 1U] = '\0';
        return;
    }

    snprintf(
        output,
        output_size,
        "search_complete results=%zu "
        "title=%s source=%s url=%s",
        search->result_count,
        search->results[0].title,
        search->results[0].source,
        search->results[0].url
    );

    output[output_size - 1U] = '\0';
}

static void finish_success(
    echoear_tool_router_t *router,
    echoear_live_search_t *search
)
{
    char result[ECHOEAR_TOOL_RESULT_MAX];
    bool completed;

    build_tool_result(
        search,
        result,
        sizeof(result)
    );

    completed =
        echoear_tool_router_complete(
            s_bridge.call_id,
            result
        );

    if (completed) {
        s_bridge.completed_count++;
        bridge_set_state(
            ECHOEAR_SEARCH_BRIDGE_STATE_COMPLETE,
            false
        );
    } else {
        s_bridge.failed_count++;
        bridge_set_state(
            ECHOEAR_SEARCH_BRIDGE_STATE_ERROR,
            false
        );
    }

    echoear_live_search_clear_completed();

    clear_active_request();

    (void)router;
}

static void finish_failure(void)
{
    bool failed;

    failed =
        echoear_tool_router_fail(
            s_bridge.call_id,
            ECHOEAR_TOOL_ROUTER_ERROR_EXECUTION_FAILED
        );

    if (failed) {
        s_bridge.failed_count++;
    }

    echoear_live_search_clear_completed();

    bridge_set_state(
        ECHOEAR_SEARCH_BRIDGE_STATE_ERROR,
        false
    );

    clear_active_request();
}

void echoear_search_bridge_init(void)
{
    memset(
        &s_bridge,
        0,
        sizeof(s_bridge)
    );

    s_bridge.state =
        ECHOEAR_SEARCH_BRIDGE_STATE_IDLE;

    s_bridge.generation = 1U;
}

void echoear_search_bridge_reset(void)
{
    echoear_search_bridge_init();
}

echoear_search_bridge_t *echoear_search_bridge_get(void)
{
    return &s_bridge;
}

void echoear_search_bridge_process(void)
{
    echoear_tool_router_t *router =
        echoear_tool_router_get();

    echoear_live_search_t *search =
        echoear_live_search_get();

    if (router == NULL ||
        search == NULL) {
        return;
    }

    /*
     * Complete or fail an already-routed search.
     */
    if (s_bridge.active) {
        if (router->state !=
                ECHOEAR_TOOL_ROUTER_STATE_EXECUTING ||
            strcmp(
                router->request.call_id,
                s_bridge.call_id
            ) != 0) {

            if (echoear_live_search_is_busy()) {
                echoear_live_search_cancel();
            }

            if (search->state ==
                    ECHOEAR_SEARCH_STATE_COMPLETE ||
                search->state ==
                    ECHOEAR_SEARCH_STATE_ERROR) {
                echoear_live_search_clear_completed();
            }

            bridge_set_state(
                ECHOEAR_SEARCH_BRIDGE_STATE_IDLE,
                false
            );

            clear_active_request();
            return;
        }

        if (search->state ==
                ECHOEAR_SEARCH_STATE_COMPLETE &&
            strcmp(
                search->request_id,
                s_bridge.search_request_id
            ) == 0) {

            finish_success(
                router,
                search
            );

            return;
        }

        if (search->state ==
                ECHOEAR_SEARCH_STATE_ERROR &&
            strcmp(
                search->request_id,
                s_bridge.search_request_id
            ) == 0) {

            finish_failure();
            return;
        }

        return;
    }

    /*
     * Bridge owns search.web only.
     * All other tools remain owned by their normal executors.
     */
    if (router->state !=
            ECHOEAR_TOOL_ROUTER_STATE_EXECUTING ||
        strcmp(
            router->request.tool_name,
            "search.web"
        ) != 0) {
        return;
    }

    if (!extract_query(
            router->request.arguments,
            s_bridge.query,
            sizeof(s_bridge.query))) {

        copy_text(
            s_bridge.call_id,
            sizeof(s_bridge.call_id),
            router->request.call_id
        );

        s_bridge.failed_count++;

        echoear_tool_router_fail(
            s_bridge.call_id,
            ECHOEAR_TOOL_ROUTER_ERROR_EXECUTION_FAILED
        );

        bridge_set_state(
            ECHOEAR_SEARCH_BRIDGE_STATE_ERROR,
            false
        );

        clear_active_request();
        return;
    }

    /*
     * Clean a terminal direct-search test state before
     * beginning a Tool Router owned request.
     */
    if (search->state ==
            ECHOEAR_SEARCH_STATE_COMPLETE ||
        search->state ==
            ECHOEAR_SEARCH_STATE_ERROR) {
        echoear_live_search_clear_completed();
    }

    copy_text(
        s_bridge.call_id,
        sizeof(s_bridge.call_id),
        router->request.call_id
    );

    copy_text(
        s_bridge.search_request_id,
        sizeof(s_bridge.search_request_id),
        router->request.call_id
    );

    if (!echoear_live_search_submit(
            s_bridge.search_request_id,
            ECHOEAR_SEARCH_TYPE_WEB,
            s_bridge.query)) {

        s_bridge.failed_count++;

        echoear_tool_router_fail(
            s_bridge.call_id,
            ECHOEAR_TOOL_ROUTER_ERROR_EXECUTION_FAILED
        );

        bridge_set_state(
            ECHOEAR_SEARCH_BRIDGE_STATE_ERROR,
            false
        );

        clear_active_request();
        return;
    }

    s_bridge.routed_count++;

    bridge_set_state(
        ECHOEAR_SEARCH_BRIDGE_STATE_ROUTING,
        true
    );
}

const char *echoear_search_bridge_state_name(
    echoear_search_bridge_state_t state
)
{
    switch (state) {
        case ECHOEAR_SEARCH_BRIDGE_STATE_IDLE:
            return "idle";

        case ECHOEAR_SEARCH_BRIDGE_STATE_ROUTING:
            return "routing";

        case ECHOEAR_SEARCH_BRIDGE_STATE_COMPLETE:
            return "complete";

        case ECHOEAR_SEARCH_BRIDGE_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}