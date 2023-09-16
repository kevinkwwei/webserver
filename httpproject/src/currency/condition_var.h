//
// Created by Kevin Wei on 2023/8/7.
//

#pragma once

#include <exception>
#include <pthread.h>

class ConditionVariable {
public:
    ConditionVariable() {
        int ret = pthread_cond_init(&cond_, NULL);
        if (ret != 0) {
            throw std::exception();
        }
    }

    ~ConditionVariable() {
        pthread_cond_destroy(&cond_);
    }

    bool Wait(pthread_mutex_t *mutex);
    bool TimeWait(pthread_mutex_t *mutex, struct timespec t);
    bool Signal();
    bool BroadCast();
private:
    pthread_cond_t cond_;
};

