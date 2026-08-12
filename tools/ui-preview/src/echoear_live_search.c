#include "echoear_live_search.h"

#include <string.h>

static echoear_live_search_t s_search;

static bool text_present(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static bool copy_text(
    char *destination,
    size_t destination_size,
    const char *source)
{
    size_t length;

    if (destination == NULL ||
        destination_size == 0U ||
        source == NULL) {
        return false;
    }

    length = strlen(source);

    if (length >= destination_size) {
        return false;
    }

    memcpy(destination, source, length + 1U);

    return true;
}

static bool valid_type(echoear_search_type_t type)
{
    return type >= ECHOEAR_SEARCH_TYPE_WEB &&
           type < ECHOEAR_SEARCH_TYPE_COUNT;
}

static void bump_generation(void)
{
    s_search.generation++;
}

static void set_state(
    echoear_search_state_t state,
    echoear_search_error_t error)
{
    s_search.state = state;
    s_search.error = error;
    bump_generation();
}

static bool request_matches(const char *request_id)
{
    return text_present(request_id) &&
           text_present(s_search.request_id) &&
           strcmp(request_id, s_search.request_id) == 0;
}

static void clear_request(void)
{
    s_search.request_active = false;

    s_search.request_id[0] = '\0';
    s_search.query[0] = '\0';

    memset(
        s_search.results,
        0,
        sizeof(s_search.results)
    );

    s_search.result_count = 0U;
    s_search.deadline_ms = 0U;
}

static void update_availability(void)
{
    if (!s_search.config.enabled) {
        if (!echoear_live_search_is_busy()) {
            set_state(
                ECHOEAR_SEARCH_STATE_IDLE,
                ECHOEAR_SEARCH_ERROR_NONE
            );
        }
        return;
    }

    if (!s_search.network_ready) {
        if (!echoear_live_search_is_busy()) {
            set_state(
                ECHOEAR_SEARCH_STATE_OFFLINE,
                ECHOEAR_SEARCH_ERROR_NETWORK_UNAVAILABLE
            );
        }
        return;
    }

    if (!s_search.gateway_ready) {
        if (!echoear_live_search_is_busy()) {
            set_state(
                ECHOEAR_SEARCH_STATE_OFFLINE,
                ECHOEAR_SEARCH_ERROR_GATEWAY_UNAVAILABLE
            );
        }
        return;
    }

    if (!echoear_live_search_is_busy() &&
        s_search.state != ECHOEAR_SEARCH_STATE_COMPLETE &&
        s_search.state != ECHOEAR_SEARCH_STATE_ERROR) {
        set_state(
            ECHOEAR_SEARCH_STATE_READY,
            ECHOEAR_SEARCH_ERROR_NONE
        );
    }
}

echoear_search_config_t echoear_live_search_default_config(void)
{
    echoear_search_config_t config;

    memset(&config, 0, sizeof(config));

    config.enabled = true;
    config.timeout_ms = 15000U;
    config.max_results = 4U;

    copy_text(
        config.locale,
        sizeof(config.locale),
        "th-TH"
    );

    return config;
}

void echoear_live_search_init(void)
{
    memset(&s_search, 0, sizeof(s_search));

    s_search.config =
        echoear_live_search_default_config();

    s_search.state =
        ECHOEAR_SEARCH_STATE_OFFLINE;

    s_search.error =
        ECHOEAR_SEARCH_ERROR_NETWORK_UNAVAILABLE;
}

void echoear_live_search_reset(void)
{
    echoear_live_search_init();
}

echoear_live_search_t *echoear_live_search_get(void)
{
    return &s_search;
}

bool echoear_live_search_configure(
    const echoear_search_config_t *config)
{
    if (config == NULL ||
        config->max_results == 0U ||
        config->max_results >
            ECHOEAR_SEARCH_MAX_RESULTS ||
        config->timeout_ms == 0U ||
        !text_present(config->locale) ||
        strlen(config->locale) >=
            ECHOEAR_SEARCH_LOCALE_MAX) {
        return false;
    }

    if (echoear_live_search_is_busy()) {
        return false;
    }

    s_search.config = *config;

    update_availability();

    return true;
}

void echoear_live_search_set_network_ready(bool ready)
{
    if (s_search.network_ready == ready) {
        return;
    }

    s_search.network_ready = ready;
    bump_generation();

    if (!ready && echoear_live_search_is_busy()) {
        s_search.request_active = false;
        s_search.failure_count++;

        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_NETWORK_UNAVAILABLE
        );

        return;
    }

    update_availability();
}

void echoear_live_search_set_gateway_ready(bool ready)
{
    if (s_search.gateway_ready == ready) {
        return;
    }

    s_search.gateway_ready = ready;
    bump_generation();

    if (!ready && echoear_live_search_is_busy()) {
        s_search.request_active = false;
        s_search.failure_count++;

        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_GATEWAY_UNAVAILABLE
        );

        return;
    }

    update_availability();
}

bool echoear_live_search_submit(
    const char *request_id,
    echoear_search_type_t type,
    const char *query)
{
    if (!text_present(request_id) ||
        strlen(request_id) >=
            ECHOEAR_SEARCH_REQUEST_ID_MAX ||
        !valid_type(type)) {
        if (!echoear_live_search_is_busy()) {
            set_state(
                ECHOEAR_SEARCH_STATE_ERROR,
                ECHOEAR_SEARCH_ERROR_INVALID_ARGUMENT
            );
        }

        return false;
    }

    if (!text_present(query)) {
        if (!echoear_live_search_is_busy()) {
            set_state(
                ECHOEAR_SEARCH_STATE_ERROR,
                ECHOEAR_SEARCH_ERROR_EMPTY_QUERY
            );
        }

        return false;
    }

    if (strlen(query) >= ECHOEAR_SEARCH_QUERY_MAX) {
        if (!echoear_live_search_is_busy()) {
            set_state(
                ECHOEAR_SEARCH_STATE_ERROR,
                ECHOEAR_SEARCH_ERROR_INVALID_ARGUMENT
            );
        }

        return false;
    }

    if (echoear_live_search_is_busy()) {
        return false;
    }

    if (!s_search.config.enabled) {
        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_DISABLED
        );

        return false;
    }

    if (!s_search.network_ready) {
        set_state(
            ECHOEAR_SEARCH_STATE_OFFLINE,
            ECHOEAR_SEARCH_ERROR_NETWORK_UNAVAILABLE
        );

        return false;
    }

    if (!s_search.gateway_ready) {
        set_state(
            ECHOEAR_SEARCH_STATE_OFFLINE,
            ECHOEAR_SEARCH_ERROR_GATEWAY_UNAVAILABLE
        );

        return false;
    }

    clear_request();

    if (!copy_text(
            s_search.request_id,
            sizeof(s_search.request_id),
            request_id) ||
        !copy_text(
            s_search.query,
            sizeof(s_search.query),
            query)) {
        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_INTERNAL
        );

        return false;
    }

    s_search.type = type;
    s_search.request_active = true;
    s_search.request_count++;

    s_search.deadline_ms =
        s_search.now_ms +
        s_search.config.timeout_ms;

    set_state(
        ECHOEAR_SEARCH_STATE_REQUEST_PENDING,
        ECHOEAR_SEARCH_ERROR_NONE
    );

    return true;
}

bool echoear_live_search_get_pending_request(
    echoear_search_request_t *request)
{
    if (request == NULL ||
        s_search.state !=
            ECHOEAR_SEARCH_STATE_REQUEST_PENDING ||
        !s_search.request_active) {
        return false;
    }

    memset(request, 0, sizeof(*request));

    if (!copy_text(
            request->request_id,
            sizeof(request->request_id),
            s_search.request_id) ||
        !copy_text(
            request->query,
            sizeof(request->query),
            s_search.query) ||
        !copy_text(
            request->locale,
            sizeof(request->locale),
            s_search.config.locale)) {
        return false;
    }

    request->type = s_search.type;
    request->max_results =
        s_search.config.max_results;

    return true;
}

bool echoear_live_search_mark_dispatched(
    const char *request_id)
{
    if (!request_matches(request_id)) {
        /*
         * Stale dispatch acknowledgement must not
         * corrupt the active request.
         */
        return false;
    }

    if (s_search.state !=
            ECHOEAR_SEARCH_STATE_REQUEST_PENDING ||
        !s_search.request_active) {
        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_PROTOCOL
        );

        return false;
    }

    set_state(
        ECHOEAR_SEARCH_STATE_WAITING_RESPONSE,
        ECHOEAR_SEARCH_ERROR_NONE
    );

    return true;
}

bool echoear_live_search_add_result(
    const char *request_id,
    const echoear_search_result_t *result)
{
    echoear_search_result_t *destination;

    if (!request_matches(request_id)) {
        /*
         * Ignore stale/out-of-order search results.
         */
        return false;
    }

    if (result == NULL ||
        s_search.state !=
            ECHOEAR_SEARCH_STATE_WAITING_RESPONSE ||
        !s_search.request_active) {
        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_PROTOCOL
        );

        return false;
    }

    if (s_search.result_count >=
            s_search.config.max_results ||
        s_search.result_count >=
            ECHOEAR_SEARCH_MAX_RESULTS) {
        return false;
    }

    destination =
        &s_search.results[s_search.result_count];

    memset(destination, 0, sizeof(*destination));

    if (!copy_text(
            destination->title,
            sizeof(destination->title),
            result->title) ||
        !copy_text(
            destination->url,
            sizeof(destination->url),
            result->url) ||
        !copy_text(
            destination->snippet,
            sizeof(destination->snippet),
            result->snippet) ||
        !copy_text(
            destination->source,
            sizeof(destination->source),
            result->source)) {
        s_search.request_active = false;
        s_search.failure_count++;

        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_RESULT_TOO_LARGE
        );

        return false;
    }

    s_search.result_count++;
    bump_generation();

    return true;
}

bool echoear_live_search_complete(
    const char *request_id)
{
    if (!request_matches(request_id)) {
        /*
         * Ignore stale completion.
         */
        return false;
    }

    if (s_search.state !=
            ECHOEAR_SEARCH_STATE_WAITING_RESPONSE ||
        !s_search.request_active) {
        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_PROTOCOL
        );

        return false;
    }

    s_search.request_active = false;
    s_search.success_count++;
    s_search.deadline_ms = 0U;

    set_state(
        ECHOEAR_SEARCH_STATE_COMPLETE,
        ECHOEAR_SEARCH_ERROR_NONE
    );

    return true;
}

bool echoear_live_search_fail(
    const char *request_id,
    echoear_search_error_t error)
{
    if (!request_matches(request_id)) {
        /*
         * Ignore stale failure.
         */
        return false;
    }

    if (!s_search.request_active) {
        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_PROTOCOL
        );

        return false;
    }

    if (error == ECHOEAR_SEARCH_ERROR_NONE) {
        error = ECHOEAR_SEARCH_ERROR_GATEWAY_ERROR;
    }

    s_search.request_active = false;
    s_search.failure_count++;
    s_search.deadline_ms = 0U;

    set_state(
        ECHOEAR_SEARCH_STATE_ERROR,
        error
    );

    return true;
}

void echoear_live_search_cancel(void)
{
    if (!echoear_live_search_is_busy()) {
        return;
    }

    s_search.request_active = false;
    s_search.cancel_count++;
    s_search.deadline_ms = 0U;

    set_state(
        ECHOEAR_SEARCH_STATE_ERROR,
        ECHOEAR_SEARCH_ERROR_CANCELLED
    );
}

void echoear_live_search_clear_completed(void)
{
    if (s_search.state !=
            ECHOEAR_SEARCH_STATE_COMPLETE &&
        s_search.state !=
            ECHOEAR_SEARCH_STATE_ERROR) {
        return;
    }

    clear_request();

    if (!s_search.config.enabled) {
        set_state(
            ECHOEAR_SEARCH_STATE_IDLE,
            ECHOEAR_SEARCH_ERROR_NONE
        );

        return;
    }

    if (!s_search.network_ready) {
        set_state(
            ECHOEAR_SEARCH_STATE_OFFLINE,
            ECHOEAR_SEARCH_ERROR_NETWORK_UNAVAILABLE
        );

        return;
    }

    if (!s_search.gateway_ready) {
        set_state(
            ECHOEAR_SEARCH_STATE_OFFLINE,
            ECHOEAR_SEARCH_ERROR_GATEWAY_UNAVAILABLE
        );

        return;
    }

    set_state(
        ECHOEAR_SEARCH_STATE_READY,
        ECHOEAR_SEARCH_ERROR_NONE
    );
}

void echoear_live_search_tick(uint32_t now_ms)
{
    s_search.now_ms = now_ms;

    if (!echoear_live_search_is_busy() ||
        s_search.deadline_ms == 0U) {
        return;
    }

    if ((int32_t)(
            now_ms -
            s_search.deadline_ms) >= 0) {
        s_search.request_active = false;
        s_search.failure_count++;
        s_search.deadline_ms = 0U;

        set_state(
            ECHOEAR_SEARCH_STATE_ERROR,
            ECHOEAR_SEARCH_ERROR_TIMEOUT
        );
    }
}

bool echoear_live_search_is_ready(void)
{
    return s_search.config.enabled &&
           s_search.network_ready &&
           s_search.gateway_ready &&
           s_search.state ==
               ECHOEAR_SEARCH_STATE_READY;
}

bool echoear_live_search_is_busy(void)
{
    return s_search.request_active ||
           s_search.state ==
               ECHOEAR_SEARCH_STATE_REQUEST_PENDING ||
           s_search.state ==
               ECHOEAR_SEARCH_STATE_WAITING_RESPONSE;
}

const char *echoear_live_search_state_name(
    echoear_search_state_t state)
{
    switch (state) {
        case ECHOEAR_SEARCH_STATE_IDLE:
            return "idle";

        case ECHOEAR_SEARCH_STATE_OFFLINE:
            return "offline";

        case ECHOEAR_SEARCH_STATE_READY:
            return "ready";

        case ECHOEAR_SEARCH_STATE_REQUEST_PENDING:
            return "request_pending";

        case ECHOEAR_SEARCH_STATE_WAITING_RESPONSE:
            return "waiting_response";

        case ECHOEAR_SEARCH_STATE_COMPLETE:
            return "complete";

        case ECHOEAR_SEARCH_STATE_ERROR:
            return "error";

        default:
            return "unknown";
    }
}

const char *echoear_live_search_error_name(
    echoear_search_error_t error)
{
    switch (error) {
        case ECHOEAR_SEARCH_ERROR_NONE:
            return "none";

        case ECHOEAR_SEARCH_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case ECHOEAR_SEARCH_ERROR_DISABLED:
            return "disabled";

        case ECHOEAR_SEARCH_ERROR_NETWORK_UNAVAILABLE:
            return "network_unavailable";

        case ECHOEAR_SEARCH_ERROR_GATEWAY_UNAVAILABLE:
            return "gateway_unavailable";

        case ECHOEAR_SEARCH_ERROR_EMPTY_QUERY:
            return "empty_query";

        case ECHOEAR_SEARCH_ERROR_BUSY:
            return "busy";

        case ECHOEAR_SEARCH_ERROR_TIMEOUT:
            return "timeout";

        case ECHOEAR_SEARCH_ERROR_STALE_RESPONSE:
            return "stale_response";

        case ECHOEAR_SEARCH_ERROR_TOO_MANY_RESULTS:
            return "too_many_results";

        case ECHOEAR_SEARCH_ERROR_RESULT_TOO_LARGE:
            return "result_too_large";

        case ECHOEAR_SEARCH_ERROR_GATEWAY_ERROR:
            return "gateway_error";

        case ECHOEAR_SEARCH_ERROR_PROTOCOL:
            return "protocol";

        case ECHOEAR_SEARCH_ERROR_CANCELLED:
            return "cancelled";

        case ECHOEAR_SEARCH_ERROR_INTERNAL:
            return "internal";

        default:
            return "unknown";
    }
}

const char *echoear_live_search_type_name(
    echoear_search_type_t type)
{
    switch (type) {
        case ECHOEAR_SEARCH_TYPE_WEB:
            return "web";

        case ECHOEAR_SEARCH_TYPE_NEWS:
            return "news";

        case ECHOEAR_SEARCH_TYPE_LIVE_INFO:
            return "live_info";

        default:
            return "unknown";
    }
}