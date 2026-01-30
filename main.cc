#include <drogon/drogon.h>
#include "utils/response.h"

int main() {
    // Set HTTP listener address and port
    //  drogon::app().addListener("0.0.0.0", 5555);
    // Load config file

    // 错误处理
    drogon::app().setExceptionHandler(
        [](const std::exception &e, const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
            using RE = response::ResponsiveException;

            if (const auto *re = dynamic_cast<const RE *>(&e)) {
                cb(re->resp());
            } else {
                cb(response::fail(drogon::k500InternalServerError, "服务器错误"));
            }
        });

    drogon::app().loadConfigFile("config.json");
    // drogon::app().loadConfigFile("../config.yaml");
    // Run HTTP framework,the method will block in the internal event loop
    drogon::app().run();
    return 0;
}
