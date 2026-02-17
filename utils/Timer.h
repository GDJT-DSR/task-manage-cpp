#pragma once
#include <chrono>
#include <thread>

class Timer
{
    std::chrono::seconds dur_;

    std::thread worker_;
    std::function<void()> func_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};

public:
    explicit Timer(std::chrono::seconds);

    void start(std::function<void()>&&);

    ~Timer();
};
