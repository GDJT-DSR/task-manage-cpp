//
// Created by dsr on 2026/2/20.
//

#pragma once
#include <drogon/plugins/Plugin.h>

namespace dsr
{
    namespace password_encoder
    {
        class PasswordEncoderBase
        {
        public:
            virtual ~PasswordEncoderBase() = default;
            [[nodiscard]] virtual std::string encode(std::string_view input) const = 0;
            [[nodiscard]] virtual bool verify(std::string_view raw, std::string_view encoded) const = 0;
        };

        template <typename Impl>
        class PasswordEncoder : public PasswordEncoderBase, public drogon::DrObject<Impl>
        {
            PasswordEncoder() = default;
            friend Impl;
        };
    }

    namespace plugin
    {
        class PasswordEncoder : public drogon::Plugin<PasswordEncoder>, public password_encoder::PasswordEncoderBase
        {
        public:
            void initAndStart(const Json::Value& config) final;
            void shutdown() final;
            [[nodiscard]] std::string encode(std::string_view input) const final;
            [[nodiscard]] bool verify(std::string_view raw, std::string_view encoded) const final;

        private:
            std::shared_ptr<PasswordEncoderBase> implement_ptr_{nullptr};
        };
    }
}

