#include "App.h"
#include <Bulwark.h>

void handle_home(Request_Context& ctx) {
    ctx.serve_file("web/index.html");
}
void handle_login(Request_Context& ctx) {
    ctx.serve_file("web/login.html");
}
void handle_register(Request_Context& ctx) {
    ctx.serve_file("web/register.html");
}


int main(void) {
    App_State app;
    app.port = "8000";
    app.static_dir = "web/public";

    app.get_routes["/"] = { .handler = handle_home, .middleware = {} };
    app.get_routes["/login"] = { .handler = handle_login, .middleware = {} };
    app.get_routes["/register"] = { .handler = handle_register, .middleware = {} };

    Run_App(app);

    return 0;
}
