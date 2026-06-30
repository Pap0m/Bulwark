#include "mongoose.h"
#include <bulwark.h>
#include <sqlite3.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

typedef struct route route_t;

typedef struct {
    string_t method;
    void (*handler)(Context *ctx);
} s_route;

struct router {
    // key = uri
    struct { char *key; s_route value; } *static_routes;
    // hashmap static routes
    // list dynamic routes
};

struct app {
    sqlite3 *db;
    config_t config;
    struct mg_mgr mgr;

    struct router *router;
};


void ev_handler(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        app_t *app = (app_t *)conn->fn_data;

        char *path = strndup(hm->uri.buf, hm->uri.len);

        ptrdiff_t index = shgeti(app->router->static_routes, path);

        if (index != -1) {
            s_route route = app->router->static_routes[index].value;

            if (mg_strcasecmp(route.method, hm->method) == 0) {
                Context ctx = {
                    .conn = conn,
                    .req = { .raw = hm },
                };

                route.handler(&ctx);
            } else {
                mg_http_reply(conn, 405, "", "Method Not Allowed");
            }
        } else {
            mg_http_reply(conn, 404, "", "Not Found");
        }
        free(path);
    }
}

app_t* app_new_impl(config_t config) {
    app_t *app = calloc(1, sizeof(app_t));
    if (!app) goto error;

    app->config = config;

    app->router = calloc(1, sizeof(router_t));
    if (!app->router) goto error;
    sh_new_arena(app->router->static_routes);

    mg_mgr_init(&app->mgr);

    return app;

error:
    app_free_impl(app);
    return NULL;
}

void app_free_impl(app_t *app) {
    if (app) {
        if (app->router->static_routes) shfree(app->router->static_routes);
        if (app->router) free(app->router);
        if (app->db) sqlite3_close_v2(app->db);
        mg_mgr_free(&app->mgr);
        free(app);
    }
}

int app_run_impl(app_t *app) {
    char url[64];
    snprintf(url, sizeof(url), "%.*s:%.*s",
        (int)app->config.url.len, app->config.url.buf,
        (int)app->config.port.len, app->config.port.buf);

    mg_http_listen(&app->mgr, url, ev_handler, app);

    while (1) { mg_mgr_poll(&app->mgr, 1000); }
    return 0;
}

void router_get_impl(app_t *app, route_kind_t r_kind, const char *path, void (*handler)(Context *ctx)) {
    if (path == NULL || handler == NULL) return;

    switch (r_kind) {
        case STATIC:
            shput(app->router->static_routes, path, ((s_route){.method = mg_str("GET"), .handler = handler}));
            break;
        case DYNAMIC:
            assert(0 && "TODO");
        default:
            assert(0 && "Unreachable");
            break;
    }
}

void ctx_reply_impl(Context *ctx, int status, const char *headers, const char *body) {
    mg_http_reply(ctx->conn, status, headers ? headers : "", "%s", body ? body : "");
}

void ctx_json_impl(Context *ctx, int status, const char *json_string) {
    mg_http_reply(ctx->conn, status, "Content-Type: application/json\r\n", "%s", json_string);
}

string_t ctx_get_var_impl(Context *ctx, const char *name) {
    string_t var = {0};
    mg_http_get_var(&ctx->req.raw->query, name, var.buf, var.len);
    return var;
}
