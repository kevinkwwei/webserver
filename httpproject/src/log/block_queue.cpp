//
// Created by Kevin Wei on 2023/9/16.
//

#include "block_queue.h"
#include <sys/time.h>

template<class T>
void BlockQueue<T>::Clear() {
    locker_.Lock();
    max_size_ = 0;
    size_ = 0;
    front_ = -1;
    back_ = -1;
    array_ = new T[max_size_];
    locker_.UnLock();
}

template<class T>
BlockQueue<T>::BlockQueue(int max_capacity) {
    if (max_capacity < 0) {
        exit(-1);
    }
    max_size_ = max_capacity;
    array_ = new T[max_size_];
    size_ = 0;
    front_ = -1;
    back_ = -1;
}

template<class T>
bool BlockQueue<T>::Front(T &value) {
    locker_.Lock();
    if (size_ == 0) {
        locker_.UnLock();
        return false;
    }
    value = array_[front_];
    locker_.UnLock();
    return true;
}

template<class T>
bool BlockQueue<T>::IsFull() {
    locker_.Lock();
    if (size_ >= max_size_) {
        locker_.UnLock();
        return true;
    }
    locker_.UnLock();
    return false;
}

template<class T>
bool BlockQueue<T>::IsEmpty() {
    locker_.Lock();
    if (size_ == 0) {
        locker_.UnLock();
        return true;
    }
    locker_.UnLock();
    return false;
}

template<class T>
bool BlockQueue<T>::Back(T &value) {
    locker_.Lock();
    if (size_ == 0) {
        locker_.UnLock();
        return false;
    }
    value = array_[back_];
    locker_.UnLock();
    return true;
}

template<class T>
int BlockQueue<T>::Size() {
    int temp = 0;
    locker_.Lock();
    temp = size_;
    locker_.UnLock();
    return temp;
}

template<class T>
int BlockQueue<T>::MaxSize() {
    return max_size_;
}

template<class T>
bool BlockQueue<T>::Push(T &value) {
    locker_.Lock();
    if (size_ >= max_size_) {
        cond_.BroadCast();
        locker_.UnLock();
        return false;
    }
    back_++;
    array_[back_] = value;
    size_++;
    cond_.BroadCast();
    locker_.UnLock();
    return true;
}

template<class T>
bool BlockQueue<T>::Pop(T &value) {
    locker_.Lock();
    while (size_ <= 0) {
        if (!cond_.Wait(locker_.get())) {
            locker_.UnLock();
            return false;
        }
    }
    front_++;
    value = array_[front_];
    size_--;
    locker_.UnLock();
    return true;
}

template<class T>
bool BlockQueue<T>::Pop(T &value, int time_out) {
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    locker_.Lock();
    if (size_ <= 0) {
        t.tv_sec = now.tv_sec + time_out / 1000;
        t.tv_nsec = (time_out % 1000) * 1000;
        if (!cond_.TimeWait(locker_.get(), t)) {
            locker_.UnLock();
            return false;
        }
    }
    if (size_ <= 0) {
        locker_.UnLock();
        return false;
    }

    front_ = (front_ + 1) % max_size_;
    value = array_[front_];
    size_--;
    locker_.UnLock();
    return true;
}

template<class T>
BlockQueue<T>::~BlockQueue() {
    locker_.Lock();
    if (array_ != NULL) {
        delete[] array_;
    }
    locker_.UnLock();
}
