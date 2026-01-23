#include <thread>
#include <chrono>

namespace timer
{
    class interval
    {
    private:
        using exectuable = std::function<void()>;
        std::thread worker;
        std::atomic<bool> started;
        std::chrono::seconds duration;
        exectuable task;

        std::mutex mtx;
        std::condition_variable cv;
        void daemon(bool);
        void single();

    public:
        interval(exectuable &&task, std::chrono::seconds duration) : task(std::move(task)), duration(duration)
        {
        }
        bool start(bool = true);
    };
} // namespace timer
