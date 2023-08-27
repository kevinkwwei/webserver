//
// Created by Kevin Wei on 2023/8/27.
//
#pragma once

#include <iostream>
#include <list>
#include "currency/mutex_locker.h"
#include "currency/semaphore.h"

template <typename T>
class ThreadPool : std::enable_shared_from_this<ThreadPool> {
public:
    ThreadPool(int actor_model, int thread_count, int max_request_count);
    ~ThreadPool();
    bool Append(T *request, int state);
    bool Append(T *request);
private:
    static void *Worker(void *arg);
    void Run();

    int64_t thread_count_;
    int64_t max_request_count_;
    std::list<T *> work_queue_;
    MutexLocker work_queue_lock_;
    Semaphore queue_sem_;
    pthread_t *threads_;
    int actor_model_;

};

