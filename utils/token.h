#pragma once

#include <drogon/drogon.h>
#include <stdint.h>

namespace token
{
    struct UserClaim
    {
        int64_t id;
        int64_t permission;
    };
    std::pair<std::string, std::string> generateToken(int64_t id, int64_t permission);
    drogon::Task<std::unique_ptr<UserClaim>> parseAccessToken(std::string token);
} // namespace token
