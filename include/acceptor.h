#pragma once
#include "config_types.h"
#include "router.h"
#include "logger.h"
#include <atomic>
#include <thread>
#include <functional>
#include "connection_pool.h"
#include "IConnection.h"
class Acceptor {
public:
    using AcceptCallback = std::function<void(std::shared_ptr<IConnection> conn, int clientFd, const BackendConfig& backend)>;

    Acceptor(const ListenConfig& listenConfig,
             Router& router,
             ILogger& logger,
             ConnectionPool& connectionPool,
             AcceptCallback onAccept);

    ~Acceptor();

    void start();

    void stop();
    bool isRunning() const noexcept { return m_Running.load(); }
    void onConnectionClosed(std::shared_ptr<IConnection> conn);
private:
    void acceptLoop();
    void setupListeningSocket();
    void closeListeningSocket();
    ConnectionPool& m_ConnectionPool;
    int m_ServerFd{-1};
    std::string m_Host;
    uint16_t m_Port;
    int m_Backlog;

    std::atomic<bool> m_Running{false};
    std::thread m_Thread;

    Router& m_Router;
    ILogger& m_Logger;
    AcceptCallback m_OnAcceptCallback;

    int m_AcceptErrorCount{0};
};
