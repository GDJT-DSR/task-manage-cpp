//
// Created by dsr on 2026/1/27.
//

#include "Timer.h"


Timer::Timer(std::chrono::seconds dur) : dur_(dur)
{
}

void Timer::start(std::function<void()>&& f)
{
    if (running_)
    {
        return;
    }
    func_ = std::move(f);
    running_ = true;
    worker_ = std::thread([this]()
    {
        func_();
        while (running_)
        {
            std::unique_lock lock(mtx_);
            if (cv_.wait_for(lock, dur_) == std::cv_status::timeout)
            {
                func_();
            }
            // std::this_thread::sleep_for(dur_);
        }
    });
}

Timer::~Timer()
{
    if (!worker_.joinable())
    {
        return;
    }
    if (running_)
    {
        running_ = false;
        cv_.notify_all();
        worker_.join();
    }
    else
    {
        cv_.notify_all();
        worker_.join();
    }
}
