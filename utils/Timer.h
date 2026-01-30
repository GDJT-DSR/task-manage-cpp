#pragma once
#include <chrono>
#include <thread>

class Timer {
    std::chrono::seconds dur;

    std::atomic<bool> active = false;
    std::thread worker;
    std::function<void()> func;
    std::mutex mtx;
    std::condition_variable cv;

public:
    Timer(std::chrono::seconds dur) : dur(dur) {
    }

    void start(std::function<void()> &&f) {
        if (this->active) {
            return;
        }
        this->func = std::move(f);
        this->active = true;
        this->worker = std::thread([this]() {
            while (true) {
                this->func();
                std::this_thread::sleep_for(dur);
            }
        });
        this->worker.detach();
    };
};
