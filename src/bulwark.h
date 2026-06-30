#pragma once

#ifndef BULWARK_H_
#define BULWARK_H_

#include <mongoose.h>
#include <sqlite3.h>

typedef const enum { STATIC, DYNAMIC } route_kind_t;

typedef struct app app_t;
typedef struct router router_t;
typedef struct mg_str string_t;

typedef struct Context Context;
typedef struct Request Request;
typedef struct Response Response;

typedef struct {
    string_t url;
    string_t port;
    string_t db_path;
} config_t;

// app functions
app_t* app_new_impl(config_t config);
void   app_free_impl(app_t *app);
int    app_run_impl(app_t *app);

// router functions
router_t* app_get_router_impl(app_t *app);
void      router_get_impl(app_t *app, route_kind_t r_kind, const char *path, void (*handler)(Context *ctx));

static const struct {
    // app_t
    app_t* (*new)(config_t config);
    int (*run)(app_t *self);
    void (*free)(app_t *self);

    // route_t

    void (*get)(app_t *app, route_kind_t r_kind, const char *path, void (*handler)(Context *ctx));
} App = {
    .new = app_new_impl,
    .run = app_run_impl,
    .free = app_free_impl,
    .get = router_get_impl
};

struct Request {
    struct mg_http_message *raw;
};

struct Context {
    struct mg_connection *conn;
    Request req;
};

// In bulwark.h

void ctx_reply_impl(Context *ctx, int status, const char *headers, const char *body);
void ctx_json_impl(Context *ctx, int status, const char *json_string);
string_t ctx_get_var_impl(Context *ctx, const char *name);

static const struct {
    void (*reply)(Context *ctx, int status, const char *headers, const char *body);
    void (*json)(Context *ctx, int status, const char *json_string);
    string_t (*get_var)(Context *ctx, const char *name);
} Ctx = {
    .reply = ctx_reply_impl,
    .json = ctx_json_impl,
    .get_var = ctx_get_var_impl
};

#endif // BULWARK_H_
