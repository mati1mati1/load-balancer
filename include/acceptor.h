#pragma once
#include "config_types.h"
#include "router.h"
#include "logger.h"
#include <atomic>
#include <thread>
#include <functional>

class Acceptor {
public:
    using AcceptCallback = std::function<void(int clientFd, const BackendConfig& backend)>;

    Acceptor(const ListenConfig& listenConfig,
             Router& router,
             ILogger& logger,
             AcceptCallback onAccept);

    ~Acceptor();

    void start();

    void stop();
    bool isRunning() const noexcept { return m_Running.load(); }

private:
    void acceptLoop();
    void setupListeningSocket();
    void closeListeningSocket();

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
