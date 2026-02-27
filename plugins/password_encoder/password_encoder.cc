//
// Created by dsr on 2026/2/20.
//

#include "password_encoder.h"

#include <drogon/HttpAppFramework.h>

using namespace dsr::plugin;

void PasswordEncoder::initAndStart(const Json::Value& config)
{
    const std::string& implement = config["implement"].asString();
    if (implement.empty())
    {
        throw std::runtime_error(std::format("implement of PasswordEncoder is not set", implement));
    }
    const std::shared_ptr<DrObjectBase> ptr = drogon::DrClassMap::getSingleInstance(implement);
    if (!ptr) { throw std::runtime_error(std::format("{} is not exist", implement)); }
    implement_ptr_ = std::dynamic_pointer_cast<PasswordEncoderBase>(ptr);
    if (!implement_ptr_)
    {
        throw std::runtime_error(std::format("{} is not inherited from PasswordEncoderBase", implement));
    }
    LOG_INFO << std::format("use {} for password encoder", implement);
}

void PasswordEncoder::shutdown() { implement_ptr_ = nullptr; }
std::string PasswordEncoder::encode(const std::string_view input) const { return implement_ptr_->encode(input); }

bool PasswordEncoder::verify(const std::string_view raw, const std::string_view encoded) const
{
    return implement_ptr_->verify(raw, encoded);
}
