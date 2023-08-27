//
// Created by Kevin Wei on 2023/8/27.
//

#include "thread_pool.h"

template<typename T>
ThreadPool<T>::ThreadPool(int actor_model, int thread_count, int max_request_count) : actor_model_(actor_model),
                                                                                      thread_count_(thread_count),
                                                                                      max_request_count_(
                                                                                              max_request_count) {
    if (thread_count <= 0 || max_request_count <= 0) {
        throw std::exception();
    }
    threads_ = new pthread_t[thread_count_];
    for (int i = 0; i < thread_count_; i++) {
        auto ret = pthread_create(threads_ + i, NULL, Worker, this);
        if (ret != 0) {
            std::cout << "ThreadPool--thread create is failed" << std::endl;
            delete[] threads_;
            throw std::exception();
        }
        auto detach_ret = pthread_detach(threads_[i]);
        if (detach_ret) {
            delete[] threads_;
            throw std::exception();
        }
    }
}

template<typename T>
ThreadPool<T>::~ThreadPool<T>() {
    delete[] threads_;
}

template<typename T>
bool ThreadPool<T>::Append(T *request, int state) {
    work_queue_lock_.Lock();
    if (work_queue_.size() == max_request_count_) {
        work_queue_lock_.UnLock();
        return false;
    }
    request->state_ = state;
    work_queue_.push_back(request);
    work_queue_lock_.UnLock();
    queue_sem_.Post();
    return true;
}

template<typename T>
bool ThreadPool<T>::Append(T *request) {
    work_queue_lock_.Lock();
    if (work_queue_.size() == max_request_count_) {
        work_queue_lock_.UnLock();
        return false;
    }
    work_queue_.push_back(request);
    work_queue_lock_.UnLock();
    queue_sem_.Post();
    return true;
}

template<typename T>
void *ThreadPool<T>::Worker(void *arg) {
    ThreadPool *pool = (ThreadPool *) arg;
    pool->run();
    return pool;
}


template<typename T>
void ThreadPool<T>::Run() {
    while (true) {
        queue_sem_.Wait();
        work_queue_lock_.Lock();
        if (work_queue_.empty()) {
            work_queue_lock_.UnLock();
            continue;
        }
        auto req = work_queue_.front();
        work_queue_.pop_front();
        work_queue_lock_.UnLock();
        if (!req) {
            continue;
        }
        if (actor_model_ == 1) {
            if (req->state_ == 0) {
                if (req->read_once()) {
                    req->improv = 1;
                    req->process();
                } else {
                    req->improv = 1;
                    req->timer_flag = 1;
                }
            } else {
                if (req->write()) {
                    req->improv = 1;
                } else {
                    req->improv = 1;
                    req->timer_flag = 1;
                }
            }
        }
    }
}
