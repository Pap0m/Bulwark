#include "mongoose.h"
#include <functional>
#include <map>
#include <sqlite3.h>
#include <string>

class Database {
private:
    sqlite3 *db = nullptr;

public:
    Database(const std::string& path) {
        sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, "");
        sqlite3_open(path.c_str(), &db);
    }
    ~Database() {
        if (!db) {
            sqlite3_close_v2(db);
        }
    }
};

struct Server_Config {
    std::string url = "http://0.0.0.0:8000";
    std::string db_path = "database.db";
};

struct Http_Context {
    mg_connection *connection;
    mg_http_message *hm;
    std::string method;
    std::string uri;
    std::string body;
};

using Route_Handler = std::function<void(const Http_Context&, Database&)>;

class Router {
private:
    std::map<std::string, Route_Handler> routes;

public:
    void add_route(const std::string& method, const std::string& uri, Route_Handler route_handler) {
        if (!route_handler) return;
        std::string path = method + " " + uri;
        routes.insert(std::pair(path, route_handler));
    }

    void dispatch(const Http_Context& ctx, Database& db) {
        std::string route_key = ctx.method + " " + ctx.uri;
        auto it = routes.find(route_key);

        if (it != routes.end()) {
            // exec the handler
            it->second(ctx, db);
        } else {
            mg_http_reply(ctx.connection, 404, "Content-Type: text/plain\r\n", "404 Not Found\n");
        }
    }
};

class Web_Server {
private:
    mg_mgr mgr;
    Database database;
    Router router;

    static void ev_handler(mg_connection *c, int ev, void *ev_data) {
        if (ev == MG_EV_HTTP_MSG) {
            struct mg_http_message *hm = static_cast<struct mg_http_message *>(ev_data);
            Web_Server *server = static_cast<Web_Server *>(c->fn_data);

            server->process_request(c, hm);
        }
    }

    void process_request(mg_connection *c, mg_http_message *hm) {
        Http_Context ctx;
        ctx.connection = c;
        ctx.hm = hm;
        ctx.method = std::string(hm->method.buf, hm->method.len);
        ctx.uri = std::string(hm->uri.buf, hm->uri.len);
        ctx.body = std::string(hm->body.buf, hm->body.len);

        router.dispatch(ctx, database);
    }

public:
    explicit Web_Server(Server_Config config = (Server_Config){}) : database(config.db_path) {
        mg_mgr_init(&mgr);
        mg_http_listen(&mgr, config.url.c_str(), ev_handler, this);
    }
    ~Web_Server() {
        mg_mgr_free(&mgr);
    }

    void run() {
        while (true) { mg_mgr_poll(&mgr, 1000); }
    }

    void GET(const std::string& uri, Route_Handler route_handler) {
        router.add_route("GET", uri, route_handler);
    }
    void POST(const std::string& uri, Route_Handler route_handler) {
        router.add_route("POST", uri, route_handler);
    }
    void PATCH(const std::string& uri, Route_Handler route_handler) {
        router.add_route("PATCH", uri, route_handler);
    }
    void DELETE(const std::string& uri, Route_Handler route_handler) {
        router.add_route("delete", uri, route_handler);
    }

    Router get_router() {
        return router;
    }
};

void handle_home(const Http_Context& ctx, Database &db) {
    mg_http_reply(ctx.connection, 200, "Content-Type: text/plain\r\n", "Home\n");
}
void handle_login(const Http_Context& ctx, Database &db) {

}
void handle_register(const Http_Context& ctx, Database &db) {

}

int main(void) {
    Web_Server server({ .db_path = "Bulwark.db" });

    server.GET("/", handle_home);
    server.POST("/login", handle_login);
    server.POST("/register", handle_register);

    server.run();

    return 0;
}
