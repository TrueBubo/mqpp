#include "thread_pool.hpp"
#include "ranges"

namespace mqpp {

ThreadPool::ThreadPool(size_t num_threads)
    : stop_(false)
{
    workers_.reserve(num_threads);

    for (auto&& _ : workers_) {
        workers_.emplace_back(&ThreadPool::worker_thread, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock lock(mutex_);
        stop_ = true;
    }

    conditional_variable_.notify_all();

    for (auto&& worker : workers_ | std::views::filter([](auto&& it) { return it.joinable(); })) {
        worker.join();
    }
}

void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock lock(mutex_);

            conditional_variable_.wait(lock, [this] {
                return stop_ || !tasks_.empty();
            });

            if (stop_ && tasks_.empty()) {
                return;
            }

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task) {
            task();
        }
    }
}

}  // namespace mqpp
