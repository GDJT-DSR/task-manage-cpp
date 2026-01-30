#pragma once

#include <chrono>

namespace config {
    constexpr auto ACCESS_TOKEN_EXPIRED = std::chrono::hours{1};
    constexpr auto REFRESH_TOKEN_EXPIRED = std::chrono::hours{24};
    constexpr auto JWT_KEY_EXPIRED = std::chrono::hours{6};
} // namespace config
