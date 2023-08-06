//
// Created by Kevin Wei on 2023/8/7.
//

#include "mutex_locker.h"
#include <exception>

MutexLocker::MutexLocker() {
    auto ret = pthread_mutex_init(&mutex_, NULL);
    if (ret != 0) {
        throw std::exception();
    }
}

MutexLocker::~MutexLocker() {
    pthread_mutex_destroy(&mutex_);
}

bool MutexLocker::Lock() {
    return pthread_mutex_lock(&mutex_);
}

bool MutexLocker::UnLock() {
    return pthread_mutex_unlock(&mutex_);
}
