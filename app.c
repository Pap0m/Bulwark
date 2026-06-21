#include <stdio.h>
#include <mruby.h>            /* core types, state, class/method definition */
#include <mruby/compile.h>    /* mrb_load_string, mrb_load_file */
#include <mruby/string.h>     /* string operations */
#include <mruby/array.h>      /* array operations */
#include <mruby/hash.h>       /* hash operations */
#include <mruby/data.h>       /* wrapping C structs */
#include <mruby/class.h>      /* class inspection */
#include <mruby/value.h>      /* value type macros */
#include <mruby/irep.h>       /* loading precompiled bytecode */
#include <mruby/error.h>      /* error handling (mrb_protect etc.) */
#include <mruby/variable.h>   /* instance/class/global variables */
#include <stdlib.h>

#include "c_libs/mongoose/mongoose.h"

static const char url[] = "http://0.0.0.0:8000";

static void register_route() {

}

static void ev_handler(struct mg_connection *conn, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) { // new http request received
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        mrb_state *mrb = conn->mgr->userdata;

        struct RClass *req_class = mrb_class_get(mrb, "Request");
        mrb_value req_args[4];

        req_args[0] = mrb_str_new(mrb, hm->method.buf, hm->method.len);
        req_args[1] = mrb_str_new(mrb, hm->uri.buf, hm->uri.len);
        req_args[2] = mrb_nil_value();
        req_args[3] = mrb_str_new(mrb, hm->body.buf, hm->body.len);

        struct mrb_value req_obj = mrb_obj_new(mrb, req_class, 4, req_args);
        if (mrb->exc) {
            mrb_print_error(mrb);
            return;
        }

        struct RClass *res_class = mrb_class_get(mrb, "Response");
        struct mrb_value res_obj = mrb_obj_new(mrb, res_class, 0, NULL);
        if (mrb->exc) {
            mrb_print_error(mrb);
            return;
        }

        struct RClass *bulwark_class = mrb_class_get(mrb, "Bulwark");
        mrb_value app_class_obj = mrb_obj_value(bulwark_class);
        mrb_value router_obj = mrb_funcall(mrb, app_class_obj, "New", 0);
        mrb_funcall(mrb, router_obj, "dispatch", 2, req_obj, res_obj);
        if (mrb->exc) {
            mrb_print_error(mrb);
            return;
        }

        mrb_value status_val = mrb_funcall(mrb, res_obj, "status", 0);
        int status_code = mrb_integer(status_val);

        mrb_value headers_val = mrb_funcall(mrb, res_obj, "cheaders", 0);
        const char *headers_str = mrb_string_value_cstr(mrb, &headers_val);

        mrb_value body_val = mrb_funcall(mrb, res_obj, "body", 0);
        const char *body_str = mrb_string_value_cstr(mrb, &body_val);

        mg_http_reply(conn, status_code, headers_str, "%s", body_str);
    }
}

int main(void) {
    mrb_state *mrb = mrb_open();

    FILE *f = fopen("app.rb", "r");
    if (f == NULL) {
        fprintf(stderr, "Failed to open ruby files\n");
        return EXIT_FAILURE;
    }
    mrb_load_file(mrb, f);
    fclose(f);

    if (mrb->exc) {
        mrb_print_error(mrb);
        return EXIT_FAILURE;
    }

    // Init mongoose lib
    struct mg_mgr mgr = {0};
    mg_mgr_init(&mgr);
    mgr.userdata = mrb;

    struct mg_connection *c = mg_http_listen(&mgr, url, ev_handler, NULL);
    if (c == NULL) {
        fprintf(stderr, "Failed to start listener on %s\n", url);
        return EXIT_FAILURE;
    };

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    mrb_close(mrb);

    return EXIT_SUCCESS;
}
