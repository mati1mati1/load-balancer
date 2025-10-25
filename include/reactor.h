#pragma once
#include "event_loop.h"
#include "IConnection.h"
#include "ILogger.h"
#include <unordered_map>
#include <memory>
#include <atomic>
class Reactor {
public:
    explicit Reactor(std::unique_ptr<IEventLoop> loop, ILogger& logger)
        : m_Loop(std::move(loop)), m_Logger(logger), m_Running(false) {}
    ~Reactor();
    void run();
    void registerConnection(std::shared_ptr<IConnection> conn, int clientFd, int backendFd);
    void unregisterConnection(int fd);
    void stop();
    void handleEvent(Event& e);
    #ifdef UNIT_TEST
        IEventLoop* getEventLoopForTest() { return m_Loop.get(); }
        void injectConnectionForTest(int fd, std::shared_ptr<IConnection> conn) {
            m_Connections[fd] = std::move(conn);
        }
    #endif
private:
    std::unique_ptr<IEventLoop> m_Loop;
    std::unordered_map<int, std::shared_ptr<IConnection>> m_Connections;
    ILogger& m_Logger;
    std::atomic<bool> m_Running;
};
