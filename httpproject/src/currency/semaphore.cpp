//
// Created by Kevin Wei on 2023/8/7.
//

#include "semaphore.h"
#include <exception>

Semaphore::Semaphore() {
    auto ret = sem_init(&sem_, 0, 0);
    if (ret != 0) {
        throw std::exception();
    }
}

Semaphore::~Semaphore() {
    sem_destroy(&sem_);
}

bool Semaphore::Wait() {
    return sem_wait(&sem_) == 0;
}

bool Semaphore::Post() {
    return sem_post(&sem_) == 0;
}