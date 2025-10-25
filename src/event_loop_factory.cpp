#include "event_loop_factory.h"

#ifdef __linux__
#include "epoll_event_loop.cpp"
#elif defined(__APPLE__)
#include "kqueue_event_loop.cpp"
#else
#error "Unsupported platform"
#endif

std::unique_ptr<IEventLoop> createEventLoop() {
#ifdef __linux__
    return std::make_unique<EpollEventLoop>();
#elif defined(__APPLE__)
    return std::make_unique<KqueueEventLoop>();
#endif
}
