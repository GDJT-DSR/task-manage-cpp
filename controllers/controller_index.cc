#include "controller_index.h"

#include "drogon/HttpTypes.h"
// #include "utils/response.h"
#include <fstream>
#include <ios>

using namespace controller;
using namespace drogon;

// Add definition of your processing function here
void index::home(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback) {
    // callback(HttpResponse::newFileResponse("./html/index.html"));
    // callback(response::success(req->getPath()));
    // 1. 读取文件内容
    std::ifstream file("./html/index.html", std::ios::binary);
    if (!file.is_open()) {
        // 文件不存在，返回404
        auto resp = HttpResponse::newNotFoundResponse();
        callback(resp);
        return;
    }

    // 2. 将文件内容读入字符串
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();

    // 3. 创建响应并设置内容
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody(fileContent);

    // 4. 设置Content-Type
    resp->setContentTypeCode(drogon::CT_TEXT_HTML); // 通用二进制流

    callback(resp);
}
