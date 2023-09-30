//
// Created by Kevin Wei on 2023/9/16.
//
#ifndef LOG_H
#define LOG_H
#pragma once
#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

class Log {
public:
    static Log *GetInstance() {
        static Log instance;
        return &instance;
    }
    //  函数签名为什么要看懂！！！！！！！！！
    static void *FlushLogThread(void *args) {
        Log::GetInstance()->AsyncWriteLog();
    }

    bool Init(const char *file_name, int close_log, int log_buffer_size, int split_lines, int max_queue_size);

    void WriteLog(int level, const char *format, ...);

    void Flush(void);

private:
    Log();
    virtual ~Log();
    void *AsyncWriteLog();

    char dir_name_[128];
    char log_name_[128];
    int  max_split_line_;
    int  log_buffer_size_;
    long long curr_line_count_;
    int today_;
    FILE *fp_;
    char *buff_;
    BlockQueue<std::string> *log_queue_;
    bool is_async_;
    MutexLocker mutex_;
    int close_log_;
};

#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::GetInstance()->WriteLog(0, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::GetInstance()->WriteLog(1, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::GetInstance()->WriteLog(2, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::GetInstance()->WriteLog(3, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}

#endif
