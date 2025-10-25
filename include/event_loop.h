#pragma once
#include <vector>
#include <memory>
#include <functional>

struct Event {
    int fd;
    bool readable;
    bool writable;
    bool error;
    bool closed;
};

class IEventLoop {
public:
    using EventHandler = std::function<void(const Event&)>;

    virtual ~IEventLoop() = default;

    virtual bool registerFd(int fd, bool read, bool write) = 0;
    virtual bool unregisterFd(int fd) = 0;
    virtual int wait(std::vector<Event>& events, int timeoutMs) = 0;
    virtual void closeLoop() = 0;
    virtual void updateFd(int fd, bool wantRead, bool wantWrite) = 0;
};
