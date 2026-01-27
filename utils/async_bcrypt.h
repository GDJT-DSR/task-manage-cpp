#pragma once
#include <drogon/utils/coroutine.h>

namespace async_bcrypt {
    drogon::Task<std::string> hash(const std::string &password, int cost);

    drogon::Task<bool> verify(const std::string &raw, const std::string &hashed);
}
