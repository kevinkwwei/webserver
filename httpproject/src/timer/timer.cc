//
// Created by Kevin Wei on 2023/9/29.
//

#include "timer.h"

sort_timer_lst::sort_timer_lst() {}

sort_timer_lst::~sort_timer_lst() {
    auto curr = head;
    while (curr) {
        auto temp = curr;
        delete temp;
        curr = temp->next;
    }
}

void sort_timer_lst::add_timer(util_timer *timer) {
    if (!timer) {
        return;
    }
    if (!head) {
        head = timer;
        tail = timer;
        return;
    }
    // 加在第一个
    if (timer->expire < head->expire) {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    // 加在应该加的位置, 肯定不是第一个
    add_timer(timer, head);
}

void sort_timer_lst::del_timer(util_timer *timer) {
    if (!timer) {
        return;
    }
    if (timer == head && timer == tail) {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail) {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    auto next = timer->next;
    auto prev = timer->prev;
    prev->next = next;
    next->prev = prev;
    delete timer;
}

void sort_timer_lst::adjust_timer(util_timer *timer) {
    if (!timer) {
        return;
    }
    auto temp = timer->next;
    if (!temp || timer->expire < temp->expire) {
        return;
    }
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    } else {
        auto prev = temp->prev;
        auto next = temp->next;
        prev->next = next;
        next->prev = prev;
        // timer->next 一定在timer之前，从这里开始添加也可以，也可以用head
        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::tick() {
    if (!head) {
        return;
    }
    // 定时移除链表上的过气节点
    time_t curr = time(NULL);
    auto temp = head;
    while (temp) {
        if (curr < temp->expire) {
            break;
        }
        temp->cb_func(temp->user_data);
        head = temp->next;
        temp->next = NULL;
        if (head) {
            head->prev = NULL;
        }
        delete temp;
        temp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head) {
    auto prev = lst_head;
    auto temp = prev->next;
    while (temp) {
        if (timer->expire < temp->expire) {
            prev->next = timer;
            timer->next = temp;
            timer->prev = prev;
            temp->prev = timer;
            break;
        }
        prev = temp;
        temp = temp->next;
    }

    if (!temp) {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void Timer_Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {

}

void Timer_Utils::init(int timeslot) {
    m_TIMESLOT = timeslot;
}

int Timer_Utils::setnonblocking(int fd) {
    auto old_option = fcntl(fd, F_GETFL);
    // 用位操作可以保持旧有状态并添加新状态，如果直接设置，会全覆盖而不是添加
    auto new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Timer_Utils::sig_handler(int sig) {
    auto curr_errno = errno;
    int msg = sig;
    // 写入信号
    send(u_pipefd[1], (char *) &msg, 1, 0);
    errno = curr_errno;
}

void Timer_Utils::addsig(int sig, void (*handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    // 设置对于信号处理的handler
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags = sa.sa_flags | SA_RESTART;
    }
    // 是这所有需要屏蔽的信号，这里的操作是把所有信号都放入信号集中
    sigfillset(&sa.sa_mask);
    // 信号是否成功
    auto ret = sigaction(sig, &sa, NULL);
    assert(ret != -1);
}

void Timer_Utils::timer_handler() {
    // 更新过期链表节点
    m_timer_lst.tick();
    // alarm信号
    alarm(m_TIMESLOT);
}

void Timer_Utils::show_error(int connfd, const char *info) {

}

int *Timer_Utils::u_pipefd = 0;
int Timer_Utils::u_epollfd = 0;

class Timer_Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Timer_Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
//    http_conn::m_user_count--;
}