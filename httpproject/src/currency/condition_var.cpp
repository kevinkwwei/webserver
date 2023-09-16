//
// Created by Kevin Wei on 2023/8/7.
//

#include "condition_var.h"

bool ConditionVariable::Wait(pthread_mutex_t *mutex) {
    int ret = 0;
    ret = pthread_cond_wait(&cond_, mutex);
    return ret == 0;
}

bool ConditionVariable::TimeWait(pthread_mutex_t *mutex, struct timespec t) {
    int ret = 0;
    ret = pthread_cond_timedwait(&cond_, mutex, &t);
    return ret == 0;
}

bool ConditionVariable::Signal() {
    return pthread_cond_signal(&cond_) == 0;
}

bool ConditionVariable::BroadCast() {
    return pthread_cond_broadcast(&cond_) == 0;
}