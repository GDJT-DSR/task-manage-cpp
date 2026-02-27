#pragma once

#include <drogon/drogon.h>

namespace token
{
    struct UserClaim
    {
        int id;
        int64_t permission;
    };

    std::pair<std::string, std::string> generateToken(int64_t id, int64_t permission, const std::string& updated_at);

    drogon::Task<std::optional<UserClaim>> parseAccessToken(const std::string& token);

    drogon::Task<std::optional<std::pair<int, std::string>>> parseRefreshToken(const std::string& token);
} // namespace token
