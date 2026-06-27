#pragma once

#ifndef BULWARK_ROUTER_H_
#define BULWARK_ROUTER_H_

#include <functional>
#include <vector>

struct App_State;
struct Request_Context;

using Request_Handler = std::function<void(Request_Context&)>;
using Middleware_Handler = std::function<bool(Request_Context&)>;

struct Handler {
    Request_Handler handler;
    std::vector<Middleware_Handler> middleware;
};

#endif // BULWARK_ROUTER_H_
