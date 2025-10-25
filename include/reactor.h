#pragma once
#include "event_loop.h"
#include "IConnection.h"
#include "ILogger.h"
#include "connection_pool.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <thread>
class Reactor {
public:
    explicit Reactor(std::unique_ptr<IEventLoop> loop, ILogger& logger, ConnectionPool& connectionPool)
        : m_Loop(std::move(loop)), m_Logger(logger), m_Running(false), m_ConnectionPool(connectionPool) {}
    ~Reactor();
    void run();
    void registerConnection(std::shared_ptr<IConnection> conn, int clientFd, int backendFd);
    void unregisterConnection(int fd);
    void stop();
    void handleEvent(Event& e);
    void setIdleTimeout(std::chrono::seconds timeout);
    #ifdef UNIT_TEST
        IEventLoop* getEventLoopForTest() { return m_Loop.get(); }
        void injectConnectionForTest(int fd, std::shared_ptr<IConnection> conn) {
            m_Connections[fd] = std::move(conn);
        }
    #endif
private:
    void monitorIdleConnections(); 
    std::unique_ptr<IEventLoop> m_Loop;
    std::unordered_map<int, std::shared_ptr<IConnection>> m_Connections;
    ConnectionPool& m_ConnectionPool;
    ILogger& m_Logger;
    std::atomic<bool> m_Running;
    std::chrono::seconds m_IdleTimeout{0};
    std::thread m_IdleThread;
    std::atomic<bool> m_StopIdleMonitor{false};
};
