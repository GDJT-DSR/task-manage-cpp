#include "timer.h"
#include <trantor/utils/Logger.h>

void timer::interval::daemon(bool immediate)
{
    if (immediate)
    {
        this->single();
    }
    while (1)
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (cv.wait_for(lock, this->duration) == std::cv_status::timeout)
            {
                break; // 定时器被停止
            }
        }
        this->single();
    }
}
void timer::interval::single()
{
    try
    {
        this->task();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "error occured in timer:" << e.what();
    }
}

bool timer::interval::start(bool immediate)
{
    if (this->started)
    {
        return false;
        ;
    }
    this->started = true;
    this->worker = std::thread([this, immediate]()
                               { this->daemon(immediate); });
    return true;
}