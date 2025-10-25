#include "reactor.h"
#include "event_loop_factory.h"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
Reactor::~Reactor() {
    stop();
}

void Reactor::registerConnection(std::shared_ptr<IConnection> conn, int clientFd, int backendFd) {
    m_Connections[clientFd] = conn;
    m_Connections[backendFd] = conn;
    m_Loop->registerFd(clientFd, true, false);
    m_Loop->registerFd(backendFd, true, true);
    m_Logger.logInfo("Registered connection: clientFd=" + std::to_string(clientFd) +
                  " backendFd=" + std::to_string(backendFd));
}

void Reactor::unregisterConnection(int fd) {
    m_Loop->unregisterFd(fd);
    m_Connections.erase(fd);
    m_Logger.logDebug("Unregistered fd=" + std::to_string(fd));
}

void Reactor::run() {
    m_Running = true;
    m_Logger.logInfo("Reactor started");

    std::vector<Event> events;

    while (m_Running) {
        int n = m_Loop->wait(events, 1000);
        if (n <= 0) continue;

        for (auto& e : events)
            handleEvent(e);
    }

    m_Logger.logInfo("Reactor stopped");
}

void Reactor::handleEvent(Event& e) {
    auto it = m_Connections.find(e.fd);
    if (it == m_Connections.end()) return;
    auto conn = it->second;

    if (e.error || e.closed) {
        m_Logger.logDebug("Error/Close event on fd=" + std::to_string(e.fd));
        m_Logger.logDebug("Error: " + std::string(strerror(errno)));
        conn->onClose(e.fd);
        unregisterConnection(e.fd);
        return;
    }

    if (e.writable) {
        if (!conn->isConnected() && e.fd == conn->getBackendFd()) {
            int err = 0;
            socklen_t len = sizeof(err);
            getsockopt(e.fd, SOL_SOCKET, SO_ERROR, &err, &len);
            if (err != 0) {
                m_Logger.logError("Backend connection failed: " + std::string(strerror(err)));
                conn->onClose(e.fd);
                unregisterConnection(e.fd);
                return;
            }

            conn->setConnected(true);
            m_Logger.logInfo("Backend connection established successfully (fd=" + std::to_string(e.fd) + ")");

            m_Loop->updateFd(e.fd, true, false);
        } else {
            conn->onWritable(e.fd);
        }
    }

    if (e.readable)
        conn->onReadable(e.fd);
}

void Reactor::stop() {
    m_Running = false;
    m_Loop->closeLoop();
}
