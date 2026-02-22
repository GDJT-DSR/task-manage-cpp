//
// Created by dsr on 2026/2/20.
//

#pragma once

#include "password_encoder.h"

namespace dsr::password_encoder
{
    class Md5Encoder : public PasswordEncoder<Md5Encoder>
    {
    public:
        [[nodiscard]] std::string encode(std::string_view input) const override;
        [[nodiscard]] bool verify(std::string_view raw, std::string_view encoded) const override;
    };
}
