const std = @import("std");
const Io = std.Io;
const c = @cImport({
    @cInclude("stdio.h");
    @cInclude("mongoose.h");
    @cInclude("sqlite3.h");
});

const Bulwark = @import("Bulwark");

const url = "http://0.0.0.0:8000";

const App_Context = struct {
    allocator: std.mem.Allocator,
    db: ?*c.sqlite3,
};

pub fn ev_handler(conn: ?*c.mg_connection, ev: c_int, ev_data: ?*anyopaque) callconv(.c) void {
    if (ev == c.MG_EV_HTTP_MSG) { // new http request received
        const hm: *c.mg_http_message = @ptrCast(@alignCast(ev_data));

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
        if (c.mg_match(hm.uri, c.mg_str("/assets/#"), null) or
            c.mg_match(hm.uri, c.mg_str("/thirdparty/#"), null))
        {
            const opts: c.mg_http_serve_opts = .{ .root_dir = "web/public", .fs = &c.mg_fs_posix };
            c.mg_http_serve_dir(conn, hm, &opts);
            return;
        }

        // map routes to html files
        // home
        if (c.mg_match(hm.method, c.mg_str("GET"), null) and c.mg_match(hm.uri, c.mg_str("/"), null)) {
            // const opts: c.mg_http_serve_opts = .{ .extra}
            c.mg_http_serve_file(conn, hm, "web/index.html", null);
            return;
        }
        // login
        if (c.mg_match(hm.method, c.mg_str("GET"), null) and c.mg_match(hm.uri, c.mg_str("/login"), null)) {
            c.mg_http_serve_file(conn, hm, "web/login.html", null);
            return;
        }
        // register
        if (c.mg_match(hm.method, c.mg_str("GET"), null) and c.mg_match(hm.uri, c.mg_str("/register"), null)) {
            c.mg_http_serve_file(conn, hm, "web/register.html", null);
            return;
        }
        // dashboard
        if (c.mg_match(hm.method, c.mg_str("GET"), null) and c.mg_match(hm.uri, c.mg_str("/dashboard"), null)) {
            c.mg_http_serve_file(conn, hm, "web/dashboard.html", null);
            return;
        }

        // actions and htmx routing
        // handle login
        if (c.mg_match(hm.method, c.mg_str("POST"), null) and c.mg_match(hm.uri, c.mg_str("/login"), null)) {
            std.debug.print("Body: {s}\n", .{hm.body.buf[0..hm.body.len]});

            var user_buf: [128]u8 = undefined;
            var pass_buf: [128]u8 = undefined;

            const user_len = c.mg_http_get_var(&hm.body, "username", &user_buf, user_buf.len);
            const pass_len = c.mg_http_get_var(&hm.body, "password", &pass_buf, pass_buf.len);

            if (user_len > 0 and pass_len > 0) {
                const username = user_buf[0..@intCast(user_len)];
                const password = pass_buf[0..@intCast(pass_len)];

                std.debug.print("Username: {s}\n", .{username});
                std.debug.print("Password: {s}\n", .{password});

                // TODO: Validate credentials

                c.mg_http_reply(conn, 200, "Content-Type: text/html\r\n", "<div>Login Successful!</div>");
            } else {
                c.mg_http_reply(conn, 400, "Content-Type: text/html\r\n", "<div>Login Unsuccessful!</div>");
            }

            return;
        }
        // handle register
        if (c.mg_match(hm.method, c.mg_str("POST"), null) and c.mg_match(hm.uri, c.mg_str("/register"), null)) {
            return;
        }
        // handle fetching vault items
        if (c.mg_match(hm.method, c.mg_str("GET"), null) and c.mg_match(hm.uri, c.mg_str("/items"), null)) {
            return;
        }
        // handle new vault item form
        if (c.mg_match(hm.method, c.mg_str("GET"), null) and c.mg_match(hm.uri, c.mg_str("/items/new"), null)) {
            return;
        }
        // handle adding a new valut item
        if (c.mg_match(hm.method, c.mg_str("POST"), null) and c.mg_match(hm.uri, c.mg_str("/items"), null)) {
            return;
        }

        // show independent items
        // GET /items/:id
        // GET /items/:id/edit
        // PUT /items/:id
        // DELETE /items/:id

        // session managment
        // POST logout
        if (c.mg_match(hm.method, c.mg_str("POST"), null) and c.mg_match(hm.uri, c.mg_str("/logout"), null)) {
            return;
        }

        // fallback
        // if no routes, send a 404
        c.mg_http_reply(conn, 404, "", "404 Not Found\n");
    }
}

pub fn main() !void {
    // Init sqlite3 lib
    var db: ?*c.sqlite3 = null;

    if (c.sqlite3_open("Bulwark.db", &db) != c.SQLITE_OK) {
        std.debug.print("Error: Failed to open DB\n", .{});
        return;
    }
    defer _ = c.sqlite3_close(db);

    // Init mongoose lib
    var mgr: c.mg_mgr = undefined;
    c.mg_mgr_init(&mgr);
    defer c.mg_mgr_free(&mgr);

    _ = c.mg_http_listen(&mgr, url, ev_handler, null) orelse {
        std.debug.print("Failed to start listener on {s}\n", .{url});
        return;
    };

    while (true) {
        c.mg_mgr_poll(&mgr, 1000);
    }
}
