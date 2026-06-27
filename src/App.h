#pragma once

#ifndef BULWARK_APP_H_
#define BULWARK_APP_H_

#include <vector>
#include <unordered_map>
#include <string>

#include <mongoose.h>
#include <sqlite3.h>

#include <Context.h>
#include <Router_Handler.h>

struct App_State {
    std::string port;
    std::string static_dir;

    std::unordered_map<std::string_view, Handler> get_routes;
    std::unordered_map<std::string_view, Handler> post_routes;

    std::vector<Middleware_Handler> global_middleware;

    sqlite3 *db;
};

void ev_handler(struct mg_connection *conn, int ev, void *ev_data);

void Run_App(App_State& app);

#endif // BULWARK_APP_H_
