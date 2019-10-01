#include <cstdio>
#include "threadpool.h"
#include <chrono>

void thing(int x) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::printf("Thread %d done in %s!\n", x, __func__);
}

void thing2(int x) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::printf("Thread %d done in %s!\n", x, __func__);
}

int main(void) {
    ThreadPool mytp(5, 10);

    for (int i = 0; i < 25; i++) {
        if (mytp.enqueue_job((i % 2 == 0) ? thing2 : thing, i)) {
            std::printf("Queued job %d!\n", i);
        } else {
            std::printf("Job %d failed!\n", i);
        }
    }
    
    std::printf("Sleeping for 15 seconds.\n");
    std::this_thread::sleep_for(std::chrono::seconds(15));
    return 0;
}
