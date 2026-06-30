#include <mongoose.h>
#include <stdio.h>

#include "bulwark.h"

void home_handler(Context *ctx) {
    Ctx.json(ctx, 200, "{\"status\": \"ok\", \"message\": \"Welcome to Bulwark!\"}");
}

int main(void) {
    config_t config = {
        .url = mg_str("http://0.0.0.0"),
        .port = mg_str("8000"),
        .db_path = mg_str("database.db"),
    };

    app_t *app = App.new(config);

    App.get(app, STATIC, "/", home_handler);

    App.run(app);

    return 0;
}
