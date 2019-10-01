#ifndef ECCLES_THREAD_POOL_H
#define ECCLES_THREAD_POOL_H

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <string>
#include <condition_variable>

class ThreadPool {
    public:
        ThreadPool(): thread_count(std::thread::hardware_concurrency()), max_queue(0) {
            fill_pool();
        }
        
        ThreadPool(std::size_t tcount, std::size_t queue_max = 0):
            thread_count(tcount), max_queue(queue_max) {
            fill_pool();
        }

        ~ThreadPool();
        
        template<typename... Ts>
        bool queue_job(void (*job)(Ts...), Ts... arg);

    private:
        std::size_t thread_count;
        std::size_t max_queue;
        std::vector<std::thread> tpool;
        std::queue<std::function<void()>> jobqueue;
        std::mutex tpmutex;
        std::condition_variable condition;
        std::atomic<bool> accept = true;

        void fill_pool();
};

#endif
