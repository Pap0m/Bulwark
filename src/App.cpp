#include <App.h>

void ev_handler(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        App_State *app = static_cast<App_State *>(conn->fn_data);

        Request_Context ctx = {
            .connection = conn,
            .hm = hm,
            .app = app,
        };

        bool next = true;
        for (const auto& middleware : app->global_middleware) {
            if (!middleware(ctx)) {
                next = false;
                break;
            }
        }

        if (!next) return;

        std::string_view uri(hm->uri.buf, hm->uri.len);

        // handle get funtions
        if (mg_strcmp(hm->method, mg_str("GET")) == 0) {
            auto it = app->get_routes.find(uri);
            if (it != app->get_routes.end()) {
                it->second.handler(ctx);
                return;
            } else {
                ctx.serve_dir(app->static_dir);
                return;
            }
        }
    }
}

void Run_App(App_State& app) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    std::string url = "http://0.0.0.0:" + app.port;

    struct mg_connection *c = mg_http_listen(&mgr, url.c_str(), ev_handler, &app);
    if (c == NULL) {
        fprintf(stderr, "Failed to start listener on %s\n", url.c_str());
        return;
    };

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
}
