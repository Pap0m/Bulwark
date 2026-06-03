const std = @import("std");
const Io = std.Io;
const c = @cImport({
    @cInclude("stdio.h");
    @cInclude("mongoose.h");
    @cInclude("sqlite3.h");
});

const Bulwark = @import("Bulwark");

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
        if (c.mg_match(hm.uri, c.mg_str("/assets/#"), null) or c.mg_match(hm.uri, c.mg_str("/thirdparty/#"), null)) {
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
    var mgr: c.mg_mgr = .{};
    c.mg_mgr_init(&mgr);
    defer c.mg_mgr_free(&mgr);

    _ = c.mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, null);

    while (true) {
        c.mg_mgr_poll(&mgr, 1000);
    }
}
