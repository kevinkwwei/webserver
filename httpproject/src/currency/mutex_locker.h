//
// Created by Kevin Wei on 2023/8/7.
//
#pragma once

#include <pthread.h>

class MutexLocker {
public:
    MutexLocker();

    ~MutexLocker();

    bool Lock();

    bool UnLock();

private:
    pthread_mutex_t mutex_;

};
