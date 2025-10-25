#include "event_loop.h"
#include <sys/event.h>
#include <unistd.h>

class KqueueEventLoop : public IEventLoop {
public:
    KqueueEventLoop() {
        m_Kq = kqueue();
        if (m_Kq < 0)
            throw std::runtime_error("Failed to create kqueue");
    }

    ~KqueueEventLoop() override {
        ::close(m_Kq);
    }

    bool registerFd(int fd, bool read, bool write) override {
        struct kevent evSet[2];
        int n = 0;
        if (read)
            EV_SET(&evSet[n++], fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (write)
            EV_SET(&evSet[n++], fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
        return kevent(m_Kq, evSet, n, nullptr, 0, nullptr) == 0;
    }

    bool unregisterFd(int fd) override {
        struct kevent evSet[2];
        EV_SET(&evSet[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        EV_SET(&evSet[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        kevent(m_Kq, evSet, 2, nullptr, 0, nullptr);
        return true;
    }

    int wait(std::vector<Event>& events, int timeoutMs) override {
        constexpr int MAX_EVENTS = 256;
        struct kevent evList[MAX_EVENTS];
        struct timespec ts{};
        ts.tv_sec = timeoutMs / 1000;
        ts.tv_nsec = (timeoutMs % 1000) * 1000000;
        int n = kevent(m_Kq, nullptr, 0, evList, MAX_EVENTS, &ts);
        if (n < 0) return -1;

        events.clear();
        for (int i = 0; i < n; ++i) {
            Event e;
            e.fd = evList[i].ident;
            e.readable = evList[i].filter == EVFILT_READ;
            e.writable = evList[i].filter == EVFILT_WRITE;
            e.error = evList[i].flags & EV_ERROR;
            e.closed = evList[i].flags & EV_EOF;
            events.push_back(e);
        }
        return n;
    }

    void closeLoop() override {
        ::close(m_Kq);
    }

    void updateFd(int fd, bool wantRead, bool wantWrite) override {
    struct kevent changes[2];
    int n = 0;

    EV_SET(&changes[n++], fd, EVFILT_READ, wantRead ? EV_ADD : EV_DELETE, 0, 0, nullptr);
    EV_SET(&changes[n++], fd, EVFILT_WRITE, wantWrite ? EV_ADD : EV_DELETE, 0, 0, nullptr);

    kevent(m_Kq, changes, n, nullptr, 0, nullptr);
}
private:
    int m_Kq;
};

