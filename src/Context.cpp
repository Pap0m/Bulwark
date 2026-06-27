#include <Context.h>

void Request_Context::serve_dir(const std::string& path_dir) {
    struct mg_http_serve_opts opts = {
        .root_dir = path_dir.c_str(),
        .fs = &mg_fs_posix,
    };
    mg_http_serve_dir(connection, hm, &opts);
}

void Request_Context::serve_file(const std::string& path_file) {
    struct mg_http_serve_opts opts = {
        .fs = &mg_fs_posix,
    };
    mg_http_serve_file(connection, hm, path_file.c_str(), &opts);
}
