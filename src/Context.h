#pragma once

#ifndef BULWARK_CONTEXT_H_
#define BULWARK_CONTEXT_H_

#include <string>

#include <mongoose.h>

struct App_State;

struct Request_Context {
    mg_connection *connection;
    mg_http_message *hm;
    App_State *app = nullptr; // Pointer back to the App_State so handlers can use DB

    void serve_dir(const std::string& path_dir);
    void serve_file(const std::string& path_file);
};

#endif // BULWARK_CONTEXT_H_
