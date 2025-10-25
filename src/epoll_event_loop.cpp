#include "event_loop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

class EpollEventLoop : public IEventLoop {
public:
    EpollEventLoop() {
        m_EpollFd = epoll_create1(0);
        if (m_EpollFd < 0)
            throw std::runtime_error("Failed to create epoll fd");
    }

    ~EpollEventLoop() override {
        ::close(m_EpollFd);
    }

    bool registerFd(int fd, bool read, bool write) override {
        epoll_event ev{};
        ev.data.fd = fd;
        ev.events = 0;
        if (read) ev.events |= EPOLLIN;
        if (write) ev.events |= EPOLLOUT;
        ev.events |= EPOLLET; 
        return epoll_ctl(m_EpollFd, EPOLL_CTL_ADD, fd, &ev) == 0;
    }

    bool unregisterFd(int fd) override {
        return epoll_ctl(m_EpollFd, EPOLL_CTL_DEL, fd, nullptr) == 0;
    }

    int wait(std::vector<Event>& events, int timeoutMs) override {
        constexpr int MAX_EVENTS = 256;
        epoll_event evs[MAX_EVENTS];
        int n = epoll_wait(m_EpollFd, evs, MAX_EVENTS, timeoutMs);
        if (n < 0) return -1;

        events.clear();
        for (int i = 0; i < n; ++i) {
            Event e;
            e.fd = evs[i].data.fd;
            e.readable = evs[i].events & EPOLLIN;
            e.writable = evs[i].events & EPOLLOUT;
            e.error = evs[i].events & EPOLLERR;
            e.closed = evs[i].events & EPOLLHUP;
            events.push_back(e);
        }
        return n;
    }

    void EpollEventLoop::updateFd(int fd, bool wantRead, bool wantWrite) {
        struct epoll_event ev{};
        ev.data.fd = fd;
        ev.events = 0;
        if (wantRead)  ev.events |= EPOLLIN;
        if (wantWrite) ev.events |= EPOLLOUT;
        epoll_ctl(m_EpollFd, EPOLL_CTL_MOD, fd, &ev);
    }
    
    void closeLoop() override {
        ::close(m_EpollFd);
    }

private:
    int m_EpollFd;
};
