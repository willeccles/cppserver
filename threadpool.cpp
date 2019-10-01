#include "threadpool.h"

// see https://stackoverflow.com/a/32593825/2712525

void thread_loop_fn(std::mutex* tpmut, std::condition_variable& cond,
        std::atomic<bool> accept, std::queue<std::function<void()>>& jqueue) {
    std::function<void()> tfxn;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(tpmut);

            cond.wait(lock, [&]{ return !jqueue.empty() || !accept; });

            if (!accept) {
                return;
            }
            
            tfxn = std::function<void()>(std::move(jqueue.front()));
            jqueue.pop();

            // release lock
        }
        tfxn();
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(tpmutex);
        accept = false;
    }
    condition.notify_all();
    for (auto& t : tpool) {
        t.join();
    }
}

void ThreadPool::fill_pool() {
    for (std::size_t i = 0; i < thread_count; i++) {
        tpool.push_back(std::thread(thread_loop_fn, &tpmutex, condition, accept, jobqueue));
    }
}

template<class... Ts>
bool ThreadPool::enqueue_job(void (*job)(Ts...), Ts... args) {
    {
        if (!accept) return false;
        std::unique_lock<std::mutex> lock(tpmutex);
        if (jobqueue.size() == max_queue) {
            return false;
        }
        jobqueue.push(std::bind(job, args...));
    }
    condition.notify_one();
    return true;
}
