#include "threadpool.h"

// see https://stackoverflow.com/a/32593825/2712525

void ThreadPool::thread_loop_fn() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(this->queuemutex);

            condition.wait(lock, []{ return !jobqueue.empty() || terminate; });
            // release lock
        }
    }

}

ThreadPool::~ThreadPool() {
    // TODO
}

void ThreadPool::fillpool() {
    for (std::size_t i = 0; i < this->thread_count; i++) {
        this->tpool.push_back(std::thread(thread_loop_fn));
    }
}


