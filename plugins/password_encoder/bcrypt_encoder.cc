//
// Created by dsr on 2026/2/20.
//

#include "bcrypt_encoder.h"

#include <BCrypt.hpp>

std::string dsr::password_encoder::BcryptEncoder::encode(const std::string_view input) const
{
    return BCrypt::generateHash(std::string(input), 12);
}

bool dsr::password_encoder::BcryptEncoder::verify(const std::string_view raw, const std::string_view encoded) const
{
    return BCrypt::validatePassword(std::string(raw), std::string(encoded));
}
