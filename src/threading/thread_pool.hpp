#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>

namespace mqpp {

/**
 * Thread pool for handling concurrent requests
 */
class ThreadPool {
public:
    /**
     * Create thread pool with num_threads
     * @param num_threads
     */
    explicit ThreadPool(size_t num_threads);

    /**
     * Destructor shuts down the pool and joins all threads
     */
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * Submit a task to be executed
     * @return Future with the result of the task
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * Shutdown the thread pool
     * Waits for all tasks to complete
     */
    void shutdown();

private:
    std::vector<std::thread> workers_;

    std::queue<std::function<void()>> tasks_;

    std::mutex mutex_;
    std::condition_variable conditional_variable_;

    std::atomic<bool> stop_;

    void worker_thread();
};

template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> result = task->get_future();

    {
        std::unique_lock lock(mutex_);

        if (stop_) {
            throw std::runtime_error("Cannot submit to stopped ThreadPool");
        }

        tasks_.emplace([task]() { (*task)(); });
    }

    conditional_variable_.notify_one();

    return result;
}

}  // namespace mqpp
