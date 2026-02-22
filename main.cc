#include <drogon/drogon.h>
#include <drogon/orm/Criteria.h>

#include "utils/response.h"

void beginAdvice()
{
    auto hodor = app().getSharedPlugin<plugin::Hodor>();
    hodor->setRejectResponseFactory([](const HttpRequestPtr&) {
        return response::fail(k429TooManyRequests, "Too many requests");
    });
    hodor->setUserIdGetter([](const HttpRequestPtr& req) { return req->getParameter("id"); });
}

int main()
{
    // Set HTTP listener address and port
    //  drogon::app().addListener("0.0.0.0", 5555);
    // Load config file

    // 错误处理
    app().setExceptionHandler(
        [](const std::exception& e, const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            using RE = response::ResponsiveException;

            if (const auto* re = dynamic_cast<const RE*>(&e)) { cb(re->resp()); }
            else { cb(response::fail(drogon::k500InternalServerError, "server error")); }
        });

    app().registerBeginningAdvice(beginAdvice);

    // 加载配置文件
    // drogon::app().loadConfigFile("config.json");
    app().loadConfigFile("config.yaml");
    // Run HTTP framework,the method will block in the internal event loop
    app().run();
    return 0;
}
