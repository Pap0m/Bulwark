#include <stdbool.h>
#include <stdio.h>
#include <sqlite3.h>
#include <mongoose.h>
#include <stdlib.h>
#include <string.h>

static const char url[] = "http://0.0.0.0:8000";

typedef struct {
    sqlite3 *db;
} App_Context;

sqlite3 *init_database(const char *filename) {
    sqlite3 *db = NULL;

    int ret = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return NULL;
    }
    return db;
}

void setup_schema(sqlite3 *db) {
    char *err_msg = 0;

    // Enable WAL for better concurrency
    const char *pragma_sql = "PRAGMA journal_mode=WAL;";
    sqlite3_exec(db, pragma_sql, 0, 0, &err_msg);

    // Create user table
    const char *user_schema_sql =
        "CREATE TABLE IF NOT EXISTS Users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT NOT NULL UNIQUE, "
        "master_password TEXT NOT NULL, "
        "created_at INTEGER(4) NOT NULL DEFAULT (strftime('%s', 'now')), "
        "updated_at INTEGER(4) NOT NULL DEFAULT (strftime('%s', 'now')));";

    int ret = sqlite3_exec(db, user_schema_sql, 0, 0, &err_msg);

    if (ret != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

void insert_user_data(sqlite3 *db, const char *username, const char *master_password) {
    sqlite3_stmt *stmt = {0};

    const char *sql = "INSERT INTO Users (username, master_password) VALUES (?, ?);";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(stmt, 2, master_password, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool valid_user_data() {

}

static void ev_handler(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) { // new http request received
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        App_Context *ctx = (App_Context *) ev_data;
        sqlite3 *db = ctx->db;

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
            printf("Body: %s\n", hm->body.buf);
            char user_buf[50] = "";
            char pass_buf[50] = "";
            char conf_pass_buf[50] = "";

            const int user_len = mg_http_get_var(&hm->body, "username", user_buf, sizeof(user_buf));
            const int pass_len = mg_http_get_var(&hm->body, "password", pass_buf, sizeof(pass_buf));
            const int conf_pass_len = mg_http_get_var(&hm->body, "confirm_password", conf_pass_buf, sizeof(conf_pass_buf));

            if (user_len > 0 && pass_len > 0 && conf_pass_len > 0) {
                struct mg_str username = mg_str_n(user_buf, user_len);
                struct mg_str password = mg_str_n(pass_buf, pass_len);
                struct mg_str confirm_password = mg_str_n(conf_pass_buf, conf_pass_len);

                // TODO: derivate master password
                // TODO: expand master key

                if (mg_strcmp(password, confirm_password) != 0) {
                    mg_http_reply(conn, 400, "Content-Type: text/html\r\n", "<div>Login Unsuccessful!</div>");
                }

                insert_user_data(db, username.buf, password.buf);

                mg_http_reply(conn, 200, "Content-Type: text/html\r\n", "<div>Login Successful!</div>");
            } else {
                mg_http_reply(conn, 400, "Content-Type: text/html\r\n", "<div>Login Unsuccessful!</div>");
            }
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
    sqlite3 *db = init_database("Bulwark.db");
    if (db == NULL) {
        return EXIT_FAILURE;
    }

    // init App_Context
    App_Context ctx = {0};
    ctx.db = db;

    // create sql tables
    setup_schema(db);

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
