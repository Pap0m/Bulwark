#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sqlite3.h>
#include <mongoose.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/random.h>

#include "db.h"
#include "error.h"

static const char url[] = "http://0.0.0.0:8000";

int handle_routes() {

}

typedef struct {
    sqlite3 *db;
} Context;

typedef void (*route_handler_t)(struct mg_connection *c, struct mg_http_message *hm, Context ctx);

typedef struct {
    const char *method;
    const char *uri;
    route_handler_t handler;
} Route;

void handle_static(struct mg_connection *c, struct mg_http_message *hm, Context ctx) {
    struct mg_http_serve_opts opts = { .root_dir = "web/public", .fs = &mg_fs_posix };
    mg_http_serve_dir(c, hm, &opts);
}

void handle_home_get(struct mg_connection *c, struct mg_http_message *hm, Context ctx) {
    mg_http_serve_file(c, hm, "web/index.html", NULL);
}

void handle_login_get(struct mg_connection *c, struct mg_http_message *hm, Context ctx) {
    mg_http_serve_file(c, hm, "web/login.html", NULL);
}

void handle_login_post(struct mg_connection *c, struct mg_http_message *hm, Context ctx) {
    printf("Handling login post\n");
    mg_http_reply(c, 200, "Content-Type: text/html\r\n", "<div>Login Processed</div>");
}
// The routing table.
// We use a sentinel value {NULL, NULL, NULL} at the end to know when to stop looping.
static const Route routes[] = {
    {"GET",  "/assets/#",     handle_static},
    {"GET",  "/thirdparty/#", handle_static},
    {"GET",  "/",             handle_home_get},
    {"GET",  "/login",        handle_login_get},
    {"POST", "/login",        handle_login_post},
    // {"POST", "/register",     handle_register_post},
    // {"GET",  "/items/:id", handle_get_item}, // TODO: Future implementation
    {NULL, NULL, NULL}
};

void dispatch_router(struct mg_connection *c, struct mg_http_message *hm, Context ctx) {

}

int gen_cookie(char *buf, size_t size) {
    unsigned char random_bytes[32] = {0};
    ssize_t ssize = getrandom(random_bytes, sizeof(random_bytes), GRND_NONBLOCK);
    if (ssize < 0) {
        if (ssize == EAGAIN) {
            return ERR_BLOCK_ENTROPY;
        }
        return ERR_READ_ENTROPY;
    }

    if (size < 45){
        return ERR_SMALL_BUF;
    }

    mg_base64_encode(random_bytes, sizeof(random_bytes), buf, size);

    return SUCCESS;
}

static void ev_handler(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) { // new http request received
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        Context *ctx = (Context *) conn->mgr->userdata;
        if (ctx == NULL || ctx->db == NULL) {
            fprintf(stderr, "Ctx DB lost\n");
            mg_http_reply(conn, 500, "", "Internal Server Error\n");
            return;
        }
        dispatch_router(conn, hm, *ctx);
    }
}


int main(void) {
    // Init sqlite3 lib
    sqlite3 *db = NULL;
    init_database("Bulwark.db", &db);
    if (db == NULL) {
        fprintf(stderr, "ERR: Failed to open DB\n");
        return EXIT_FAILURE;
    }

    // init Context
    Context ctx = {0};
    ctx.db = db;

    // create sql tables
    setup_schema(db);

    // Init mongoose lib
    struct mg_mgr mgr = {0};
    mg_mgr_init(&mgr);

    mgr.userdata = &ctx;

    struct mg_connection *c = mg_http_listen(&mgr, url, ev_handler, NULL);
    if (c == NULL) {
        fprintf(stderr, "Failed to start listener on %s\n", url);
        return EXIT_FAILURE;
    };

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    sqlite3_close(db);

    return EXIT_SUCCESS;
}
