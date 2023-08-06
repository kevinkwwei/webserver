//
// Created by Kevin Wei on 2023/8/7.
//
#pragma once
#include <sys/semaphore.h>

class Semaphore {
public:
    Semaphore();

    ~Semaphore();

    bool Wait();

    bool Post();

private:
    sem_t sem_;
};
