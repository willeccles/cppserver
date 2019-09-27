#ifndef ECCLES_THREAD_POOL_H
#define ECCLES_THREAD_POOL_H

#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <string>
#include <condition_variable>

class ThreadPool {
    public:
        ThreadPool(): thread_count(std::thread::hardware_concurrency()) { fill_pool(); }
        ThreadPool(std::size_t count): thread_count(count) { fill_pool(); }
        ~ThreadPool();

    private:
        std::size_t thread_count;
        std::vector<std::thread> tpool;
        std::queue<std::function<void(int, std::string)> jobqueue;
        std::mutex queuemutex;
        std::condition_variable condition;
        bool terminate = false;

        void fill_pool();
        void thread_loop_fn();
};

#endif
