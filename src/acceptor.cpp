#include "acceptor.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

Acceptor::Acceptor(const ListenConfig& listenConfig,
                   Router& router,
                   ILogger& logger,
                   AcceptCallback onAccept)
    : m_Host(listenConfig.host),
      m_Port(listenConfig.port),
      m_Backlog(listenConfig.backlog),
      m_Router(router),
      m_Logger(logger),
      m_OnAcceptCallback(std::move(onAccept))
{
    setupListeningSocket();
}

Acceptor::~Acceptor() {
    stop();
    closeListeningSocket();
}

void Acceptor::setupListeningSocket() {
    m_ServerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_ServerFd < 0) {
        perror("socket");
        throw std::runtime_error("Failed to create listening socket");
    }

    int opt = 1;
    setsockopt(m_ServerFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    fcntl(m_ServerFd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_Port);
    addr.sin_addr.s_addr = inet_addr(m_Host.c_str());

    if (::bind(m_ServerFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        throw std::runtime_error("Failed to bind listening socket");
    }

    if (listen(m_ServerFd, m_Backlog) < 0) {
        perror("listen");
        throw std::runtime_error("Failed to listen on socket");
    }

    m_Logger.logInfo("Acceptor started on " + m_Host + ":" + std::to_string(m_Port));
}

void Acceptor::start() {
    if (m_Running.exchange(true))
        return; 

    m_Thread = std::thread(&Acceptor::acceptLoop, this);
}

void Acceptor::stop() {
    if (!m_Running.exchange(false))
        return; 

    if (m_Thread.joinable())
        m_Thread.join();

    m_Logger.logInfo("Acceptor stopped on port " + std::to_string(m_Port));
}

void Acceptor::acceptLoop() {
    m_Logger.logInfo("Entering accept loop");
    while (m_Running) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        
        int clientFd = -1;

        #ifdef __linux__
            clientFd = accept4(m_ServerFd, reinterpret_cast<sockaddr*>(&clientAddr), &len, SOCK_NONBLOCK);
        #else
            clientFd = accept(m_ServerFd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
            if (clientFd >= 0) {
                int flags = fcntl(clientFd, F_GETFL, 0);
                fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
            }
        #endif
        
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (errno == EINTR)
                continue;
            m_AcceptErrorCount++;
            perror("accept4");
            continue;
        }

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
        int clientPort = ntohs(clientAddr.sin_port);

        std::string clientStr = std::string(clientIp) + ":" + std::to_string(clientPort);
        m_Logger.logInfo("Accepted connection from " + clientStr);

        try {
            auto backend = m_Router.selectBackend();
            m_OnAcceptCallback(clientFd, backend);
        } catch (const std::exception& ex) {
            m_Logger.logError(std::string("Error selecting backend: ") + ex.what());
            close(clientFd);
        }
    }
}

void Acceptor::closeListeningSocket() {
    if (m_ServerFd >= 0) {
        close(m_ServerFd);
        m_ServerFd = -1;
    }
}