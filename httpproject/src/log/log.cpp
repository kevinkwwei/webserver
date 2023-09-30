//
// Created by Kevin Wei on 2023/9/16.
//

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>

bool Log::Init(const char *file_name, int close_log, int log_buffer_size, int split_lines, int max_queue_size) {
    if (max_queue_size >= 1) {
        // 设为异步模式
        is_async_ = true;
        log_queue_ = new BlockQueue<std::string>(max_queue_size);
        pthread_t tid;
        // 这里去看解释！！！！！
        pthread_create(&tid, NULL, FlushLogThread, NULL);
    }
    // 只是一个标志位，用来标记是否打开日志系统
    close_log_ = close_log;
    log_buffer_size_ = log_buffer_size;
    buff_ = new char[log_buffer_size_];
    // '\0' is null char, the end of a string
    memset(buff_, '\0', log_buffer_size_);
    max_split_line_ = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *p = strrchr(file_name, '/'); // p指向file_name中第一个'/'
    char log_full_name[256] = {0};

    if (p == NULL) {
        // 没有'/'在里面
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                 file_name);
    } else {
        strcpy(log_name_, p + 1);
        strncpy(dir_name_, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name_, my_tm.tm_year + 1900, my_tm.tm_mon + 1,
                 my_tm.tm_mday, log_name_);
    }

    today_ = my_tm.tm_mday;
    fp_ = fopen(log_full_name, "a");
    if (!fp_) {
        return false;
    }
    return true;
}

void Log::WriteLog(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level) {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[erro]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }
    mutex_.Lock();
    curr_line_count_++;
    if (today_ != my_tm.tm_mday || curr_line_count_ % max_split_line_ == 0) //everyday log
    {

        char new_log[256] = {0};
        fflush(fp_);
        fclose(fp_);
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (today_ != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name_, tail, log_name_);
            today_ = my_tm.tm_mday;
            curr_line_count_ = 0;
        } else {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name_, tail, log_name_, curr_line_count_ / max_split_line_);
        }
        fp_ = fopen(new_log, "a");
    }

    mutex_.UnLock();

    va_list valst;
    va_start(valst, format);

    std::string log_str;
    mutex_.Lock();

    //写入的具体时间内容格式
    int n = snprintf(buff_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

    int m = vsnprintf(buff_ + n, log_buffer_size_ - n - 1, format, valst);
    buff_[n + m] = '\n';
    buff_[n + m + 1] = '\0';
    log_str = buff_;

    mutex_.UnLock();
    if (is_async_ && !log_queue_->IsFull()) {
        log_queue_->Push(log_str);
    } else {
        mutex_.Lock();
        fputs(log_str.c_str(), fp_);
        mutex_.UnLock();
    }

    va_end(valst);
}

void Log::Flush(void) {
    mutex_.Lock();
    fflush(fp_);
    mutex_.UnLock();
}

Log::Log() {
    curr_line_count_ = 0;
    is_async_ = false;
}

Log::~Log() {
    if (fp_) {
        fclose(fp_);
    }
}

void *Log::AsyncWriteLog() {
    std::string single_log;
    //从阻塞队列中取出一个日志string，写入文件
    while (log_queue_->Pop(single_log))
    {
        mutex_.Lock();
        fputs(single_log.c_str(), fp_);
        mutex_.UnLock()
    }
}
