#ifndef ECHOEAR_LIVE_SEARCH_H
#define ECHOEAR_LIVE_SEARCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHOEAR_SEARCH_REQUEST_ID_MAX     64
#define ECHOEAR_SEARCH_QUERY_MAX          256
#define ECHOEAR_SEARCH_LOCALE_MAX         16
#define ECHOEAR_SEARCH_TITLE_MAX          192
#define ECHOEAR_SEARCH_URL_MAX            384
#define ECHOEAR_SEARCH_SNIPPET_MAX        512
#define ECHOEAR_SEARCH_SOURCE_MAX         96
#define ECHOEAR_SEARCH_MAX_RESULTS        6

typedef enum {
    ECHOEAR_SEARCH_TYPE_WEB = 0,
    ECHOEAR_SEARCH_TYPE_NEWS,
    ECHOEAR_SEARCH_TYPE_LIVE_INFO,
    ECHOEAR_SEARCH_TYPE_COUNT
} echoear_search_type_t;

typedef enum {
    ECHOEAR_SEARCH_STATE_IDLE = 0,
    ECHOEAR_SEARCH_STATE_OFFLINE,
    ECHOEAR_SEARCH_STATE_READY,
    ECHOEAR_SEARCH_STATE_REQUEST_PENDING,
    ECHOEAR_SEARCH_STATE_WAITING_RESPONSE,
    ECHOEAR_SEARCH_STATE_COMPLETE,
    ECHOEAR_SEARCH_STATE_ERROR
} echoear_search_state_t;

typedef enum {
    ECHOEAR_SEARCH_ERROR_NONE = 0,
    ECHOEAR_SEARCH_ERROR_INVALID_ARGUMENT,
    ECHOEAR_SEARCH_ERROR_DISABLED,
    ECHOEAR_SEARCH_ERROR_NETWORK_UNAVAILABLE,
    ECHOEAR_SEARCH_ERROR_GATEWAY_UNAVAILABLE,
    ECHOEAR_SEARCH_ERROR_EMPTY_QUERY,
    ECHOEAR_SEARCH_ERROR_BUSY,
    ECHOEAR_SEARCH_ERROR_TIMEOUT,
    ECHOEAR_SEARCH_ERROR_STALE_RESPONSE,
    ECHOEAR_SEARCH_ERROR_TOO_MANY_RESULTS,
    ECHOEAR_SEARCH_ERROR_RESULT_TOO_LARGE,
    ECHOEAR_SEARCH_ERROR_GATEWAY_ERROR,
    ECHOEAR_SEARCH_ERROR_PROTOCOL,
    ECHOEAR_SEARCH_ERROR_CANCELLED,
    ECHOEAR_SEARCH_ERROR_INTERNAL
} echoear_search_error_t;

typedef struct {
    bool enabled;
    uint32_t timeout_ms;
    size_t max_results;
    char locale[ECHOEAR_SEARCH_LOCALE_MAX];
} echoear_search_config_t;

typedef struct {
    char request_id[ECHOEAR_SEARCH_REQUEST_ID_MAX];
    echoear_search_type_t type;
    char query[ECHOEAR_SEARCH_QUERY_MAX];
    char locale[ECHOEAR_SEARCH_LOCALE_MAX];
    size_t max_results;
} echoear_search_request_t;

typedef struct {
    char title[ECHOEAR_SEARCH_TITLE_MAX];
    char url[ECHOEAR_SEARCH_URL_MAX];
    char snippet[ECHOEAR_SEARCH_SNIPPET_MAX];
    char source[ECHOEAR_SEARCH_SOURCE_MAX];
} echoear_search_result_t;

typedef struct {
    echoear_search_state_t state;
    echoear_search_error_t error;
    echoear_search_config_t config;

    bool network_ready;
    bool gateway_ready;
    bool request_active;

    char request_id[ECHOEAR_SEARCH_REQUEST_ID_MAX];
    echoear_search_type_t type;
    char query[ECHOEAR_SEARCH_QUERY_MAX];

    echoear_search_result_t results[ECHOEAR_SEARCH_MAX_RESULTS];
    size_t result_count;

    uint32_t request_count;
    uint32_t success_count;
    uint32_t failure_count;
    uint32_t cancel_count;

    uint32_t now_ms;
    uint32_t deadline_ms;
    uint32_t generation;
} echoear_live_search_t;

echoear_search_config_t echoear_live_search_default_config(void);

void echoear_live_search_init(void);
void echoear_live_search_reset(void);

echoear_live_search_t *echoear_live_search_get(void);

bool echoear_live_search_configure(
    const echoear_search_config_t *config
);

void echoear_live_search_set_network_ready(bool ready);
void echoear_live_search_set_gateway_ready(bool ready);

bool echoear_live_search_submit(
    const char *request_id,
    echoear_search_type_t type,
    const char *query
);

bool echoear_live_search_get_pending_request(
    echoear_search_request_t *request
);

bool echoear_live_search_mark_dispatched(
    const char *request_id
);

bool echoear_live_search_add_result(
    const char *request_id,
    const echoear_search_result_t *result
);

bool echoear_live_search_complete(
    const char *request_id
);

bool echoear_live_search_fail(
    const char *request_id,
    echoear_search_error_t error
);

void echoear_live_search_cancel(void);
void echoear_live_search_clear_completed(void);

void echoear_live_search_tick(uint32_t now_ms);

bool echoear_live_search_is_ready(void);
bool echoear_live_search_is_busy(void);

const char *echoear_live_search_state_name(
    echoear_search_state_t state
);

const char *echoear_live_search_error_name(
    echoear_search_error_t error
);

const char *echoear_live_search_type_name(
    echoear_search_type_t type
);

#ifdef __cplusplus
}
#endif

#endif