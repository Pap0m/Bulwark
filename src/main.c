#include <stdio.h>
#include <sqlite3.h>
#include <mongoose.h>
#include <stdlib.h>
#include <string.h>

static const char url[] = "http://0.0.0.0:8000";

typedef struct {
    sqlite3 *db;
} App_Context;

static void ev_handler(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) { // new http request received
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        // if (c.mg_match(hm.uri, c.mg_str("/#"), null)) {
        //     const opts: c.mg_http_serve_opts = .{ .root_dir = "./web/public", .fs = &c.mg_fs_posix };
        //     c.mg_http_serve_dir(conn, hm, &opts);
        // } else {
        //     c.mg_http_reply(conn, 404, "", "%s", "Not Found\n");
        // }
        // // routes that needs authentication
        // if (c.mg_match(hm.uri, c.mg_str("/api/#"), null)) {
        //     const auth_header = c.mg_http_get_header(hm, "Authorization") orelse {
        //         c.mg_http_reply(conn, 401, "Content-Type: application/json\r\n", "{\"error\": \"Unauthorized\"}\n");
        //         return;
        //     };
        //
        //     _ = auth_header;
        // }

        // static assets
        if (mg_match(hm->uri, mg_str("/assets/#"), NULL) || mg_match(hm->uri, mg_str("/thirdparty/#"), NULL)) {
            struct mg_http_serve_opts opts = { .root_dir = "web/public", .fs = &mg_fs_posix };
            mg_http_serve_dir(conn, hm, &opts);
            return;
        }

        // map routes to html files
        // home
        if (mg_match(hm->method, mg_str("GET"), NULL) && mg_match(hm->uri, mg_str("/"), NULL)) {
            mg_http_serve_file(conn, hm, "web/index.html", NULL);
            return;
        }
        // login
        if (mg_match(hm->method, mg_str("GET"), NULL) && mg_match(hm->uri, mg_str("/login"), NULL)) {
            mg_http_serve_file(conn, hm, "web/login.html", NULL);
            return;
        }
        // register
        if (mg_match(hm->method, mg_str("GET"), NULL) && mg_match(hm->uri, mg_str("/register"), NULL)) {
            mg_http_serve_file(conn, hm, "web/register.html", NULL);
            return;
        }
        // dashboard
        if (mg_match(hm->method, mg_str("GET"), NULL) && mg_match(hm->uri, mg_str("/dashboard"), NULL)) {
            mg_http_serve_file(conn, hm, "web/dashboard.html", NULL);
            return;
        }


        // actions and htmx routing
        // handle login
        if (mg_match(hm->method, mg_str("POST"), NULL) && mg_match(hm->uri, mg_str("/login"), NULL)) {
            printf("Body: %s\n", hm->body.buf);

            char user_buf[50] = "";
            char pass_buf[50] = "";

            const int user_len = mg_http_get_var(&hm->body, "username", user_buf, sizeof(user_buf));
            const int pass_len = mg_http_get_var(&hm->body, "password", pass_buf, sizeof(pass_buf));

            if (user_len > 0 || pass_len > 0) {
                char username[50] = "";
                char password[50] = "";

                strncpy(username, user_buf, user_len);
                strncpy(password, pass_buf, pass_len);

                printf("Username: %s\n", username);
                printf("Password: %s\n", password);

                // TODO: Validate credentials

                mg_http_reply(conn, 200, "Content-Type: text/html\r\n", "<div>Login Successful!</div>");
            } else {
                mg_http_reply(conn, 400, "Content-Type: text/html\r\n", "<div>Login Unsuccessful!</div>");
            }

            return;
        }
        // handle register
        if (mg_match(hm->method, mg_str("POST"), NULL) && mg_match(hm->uri, mg_str("/register"), NULL)) {
            return;
        }
        // handle fetching vault items
        if (mg_match(hm->method, mg_str("GET"), NULL) && mg_match(hm->uri, mg_str("/items"), NULL)) {
            return;
        }
        // handle new vault item form
        if (mg_match(hm->method, mg_str("GET"), NULL) && mg_match(hm->uri, mg_str("/items/new"), NULL)) {
            return;
        }
        // handle adding a new valut item
        if (mg_match(hm->method, mg_str("POST"), NULL) && mg_match(hm->uri, mg_str("/items"), NULL)) {
            return;
        }

        // show independent items
        // GET /items/:id
        // GET /items/:id/edit
        // PUT /items/:id
        // DELETE /items/:id

        // session managment
        // POST logout
        if (mg_match(hm->method, mg_str("POST"), NULL) && mg_match(hm->uri, mg_str("/logout"), NULL)) {
            return;
        }

        // fallback
        // if no routes, send a 404
        mg_http_reply(conn, 404, "", "404 Not Found\n");
    }

}

int main(void) {
    // Init sqlite3 lib
    sqlite3 *db = NULL;

    if (sqlite3_open("Bulwark.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to open DB\n");
        return EXIT_FAILURE;
    }

    // init App_Context
    App_Context ctx = {0};
    ctx.db = db;

    // Init mongoose lib
    struct mg_mgr mgr = {0};
    mg_mgr_init(&mgr);

    struct mg_connection *c = mg_http_listen(&mgr, url, ev_handler, &ctx);
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
