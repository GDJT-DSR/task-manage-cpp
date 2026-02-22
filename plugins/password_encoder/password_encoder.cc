//
// Created by dsr on 2026/2/20.
//

#include "password_encoder.h"

#include <drogon/HttpAppFramework.h>

using namespace dsr::plugin;

void PasswordEncoder::initAndStart(const Json::Value& config)
{
    const std::string& method = config["method"].asString();
    if (method.empty()) { throw std::runtime_error(std::format("method of PasswordEncoder is not found", method)); }
    const std::shared_ptr<DrObjectBase> ptr = drogon::DrClassMap::getSingleInstance(method);
    if (!ptr) { throw std::runtime_error(std::format("{} is not exist", method)); }
    encoder_base_ = std::dynamic_pointer_cast<PasswordEncoderBase>(ptr);
    if (!encoder_base_)
    {
        throw std::runtime_error(std::format("{} is not inherited from PasswordEncoderBase", method));
    }
    LOG_INFO << std::format("use {} for password encoder", method);
}

void PasswordEncoder::shutdown() { encoder_base_ = nullptr; }
std::string PasswordEncoder::encode(const std::string_view input) const { return encoder_base_->encode(input); }

bool PasswordEncoder::verify(const std::string_view raw, const std::string_view encoded) const
{
    return encoder_base_->verify(raw, encoded);
}
