//
// Created by Kevin Wei on 2023/9/16.
//

#pragma once

#include "currency/mutex_locker.h"
#include "currency/condition_var.h"

template <class T>
class BlockQueue {
public:
    BlockQueue(int max_capacity);
    ~BlockQueue();
    bool IsFull();
    bool IsEmpty();
    void Clear();
    bool Front(T &value);
    bool Back(T &value);
    int Size();
    int MaxSize();
    bool Push(T &value);
    bool Pop(T &value);
    bool Pop(T &value, int time_out);

private:
    MutexLocker locker_;
    ConditionVariable cond_;

    T *array_;
    int size_;
    int max_size_;
    int front_;
    int back_;
};

