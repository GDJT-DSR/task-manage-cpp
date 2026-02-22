//
// Created by dsr on 2026/2/20.
//

#include "md5_encoder.h"
#include <string_view>
#include <drogon/utils/Utilities.h>

using namespace dsr::password_encoder;

std::string Md5Encoder::encode(std::string_view input) const
{
    return drogon::utils::getMd5(input.data(), input.size());
}

bool Md5Encoder::verify(std::string_view raw, std::string_view encoded) const
{
    return drogon::utils::getMd5(raw.data(), raw.size()) == encoded;
}
