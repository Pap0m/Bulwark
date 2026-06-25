#include <mongoose.h>
#include <sqlite3.h>
#include <stdlib.h>

// types
typedef struct Http_Context Http_Context;
typedef struct Request_Context Request_Context;
typedef struct Router Router;

typedef enum {
    RECEIVE_REQUEST = 0,
    HANDLE_MIDDLEWARE,
    HANDLE_REQUEST,
    SEND_RESPONSE,
    STATE_DONE,
    NUM_STATES
} State;

struct Http_Context {
    sqlite3 *db;
    int is_authorized;
    int error_code;

    struct mg_connection *c;
    struct mg_http_message *hm;
    const struct Router *matched_route;
    const char *response_body;

    State current_state;
};

typedef State (*State_Handler)(Http_Context *ctx);

typedef void (*Request_Handler)(Http_Context *ctx);

struct Router {
    const char *method;
    const char *uri;
    Request_Handler handler;
    int requires_auth;
};


State on_receive_request(Http_Context *ctx) {
    return HANDLE_MIDDLEWARE;
}
State on_handle_middleware(Http_Context *ctx) {
    if (ctx->matched_route && ctx->matched_route->requires_auth) {
        if (!ctx->is_authorized) {
            ctx->error_code = 401;
            ctx->response_body = "Unauthorized";
            return SEND_RESPONSE;
        }
    }
    return HANDLE_REQUEST;
}
State on_handle_request(Http_Context *ctx) {
    if (ctx->matched_route && ctx->matched_route->handler) {
        ctx->matched_route->handler(ctx);
    } else {
        ctx->error_code = 401;
        ctx->response_body = "Not Found";
    }
    return SEND_RESPONSE;
}
State on_send_response(Http_Context *ctx) {
    if (ctx->error_code == 0 || ctx->error_code == 200) {
        mg_http_reply(ctx->c, 200, "", "%s", ctx->response_body ? ctx->response_body : "OK");
    } else {
        mg_http_reply(ctx->c, ctx->error_code, "", "%s", ctx->response_body);
    }
    return STATE_DONE;
}

// map the functions with the actual state
State_Handler state_machine[NUM_STATES] = {
    on_receive_request,
    on_handle_middleware,
    on_handle_request,
    on_send_response,
};


struct Request_Context {
    Router router;
};

void handle_static_assets(Http_Context *ctx) {

}
void handle_home_get(Http_Context *ctx) {
    ctx->response_body = "{\"message\": \"Welcome to the public API\"}";
    ctx->error_code = 200;
}
void handle_login_post(Http_Context *ctx) {

}
void handle_dashboard_get(Http_Context *ctx) {

}

static const Router routes[] = {
    {"GET", "/public/*", handle_static_assets, 0},  // static files
    {"GET", "/", handle_home_get, 0},               // public
    {"GET", "/dashboard", handle_dashboard_get, 1}, // protected
    {"POST", "/login", handle_login_post, 0},       // public
    {NULL, NULL, NULL, 0}
};

// set the matched_route
void find_route(Http_Context *ctx) {
    for (int i = 0; routes[i].uri != NULL; ++i) {
        if (mg_strcasecmp(ctx->hm->method, mg_str(routes[i].method)) == 0) {
            struct mg_str route_uri = mg_str(routes[i].uri);
            if (mg_match(ctx->hm->uri, route_uri, NULL)) {
                // match found
                ctx->matched_route = &routes[i];
                return;
            }
        }
    }
    ctx->matched_route = NULL;
}

void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_ACCEPT) {
        Http_Context *ctx = calloc(1, sizeof(Http_Context));
        ctx->current_state = RECEIVE_REQUEST;
        c->fn_data = ctx; // attach the ctx to the connection
    }
    if (ev == MG_EV_HTTP_MSG) {
        Http_Context *ctx = (Http_Context *)c->fn_data;
        ctx->hm = (struct mg_http_message *)ev_data;
        ctx->c = c;

        find_route(ctx);
    }
    else if (ev == MG_EV_POLL) {
        Http_Context *ctx = (Http_Context *)c->fn_data;

        if (ctx && ctx->current_state != STATE_DONE) {
            ctx->current_state = state_machine[ctx->current_state](ctx);
        }
    }
    else if (ev == MG_EV_CLOSE) {
        if (c->fn_data) {
            free(c->fn_data);
            c->fn_data = NULL;
        }
    }
}

int main(void) {
    struct mg_mgr mgr = {0};

    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    return EXIT_SUCCESS;
}
