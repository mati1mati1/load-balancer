#include "connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

Connection::Connection(int clientFd, int backendFd, const BackendConfig& backend, ILogger& logger)
    : m_ClientFd(clientFd),
      m_BackendFd(backendFd),
      m_Backend(backend),
      m_Logger(logger),
      m_Connected(false),
      m_LastActivity(std::chrono::steady_clock::now())
      {
        m_Logger.logDebug("Connection created: clientFd=" + std::to_string(clientFd) +
                      ", backendFd=" + std::to_string(backendFd));
      }

Connection::~Connection() {
    closeAll();
}

bool Connection::connectToBackend() {
    m_Logger.logInfo("Connecting to backend " + m_Backend.host + ":" + std::to_string(m_Backend.port));
    if (m_Connected)
        return true;

    m_BackendFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_BackendFd < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in backendAddr{};
    backendAddr.sin_family = AF_INET;
    backendAddr.sin_port = htons(m_Backend.port);
    inet_pton(AF_INET, m_Backend.host.c_str(), &backendAddr.sin_addr);

    int result = connect(m_BackendFd, reinterpret_cast<sockaddr*>(&backendAddr), sizeof(backendAddr));

    if (result < 0) {
        if (errno == EINPROGRESS) {
            m_Logger.logInfo("Backend connection in progress (non-blocking)");
        } else {
            m_Logger.logError("Failed to connect to backend " + m_Backend.host + ":" + std::to_string(m_Backend.port) +
                              " (" + strerror(errno) + ")");
            close(m_BackendFd);
            return false;
        }
    } else {
        m_Logger.logInfo("Connected immediately to backend " + m_Backend.host + ":" + std::to_string(m_Backend.port));
        m_Connected = true;
    }

    int flags = fcntl(m_BackendFd, F_GETFL, 0);
    fcntl(m_BackendFd, F_SETFL, flags | O_NONBLOCK);

    return true;
}
void Connection::closeAll() {
    if (m_ClientFd >= 0) {
        close(m_ClientFd);
        m_ClientFd = -1;
    }
    if (m_BackendFd >= 0) {
        close(m_BackendFd);
        m_BackendFd = -1;
    }
    m_Connected = false;
}

void Connection::onReadable(int fd) {
    refreshActivity();
    m_Logger.logInfo("Readable event on fd " + std::to_string(fd));
    char buffer[8192];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        } else {
            m_Logger.logError("Recv failed on fd=" + std::to_string(fd) + " (" + strerror(errno) + ")");
            onClose(fd);
            return;
        }
    }

    if (bytesRead == 0) {
        m_Logger.logInfo("Peer closed connection on fd=" + std::to_string(fd) + ")");
        onClose(fd);
        return;
    }
    int targetFd = (fd == m_ClientFd) ? m_BackendFd : m_ClientFd;

    m_Logger.logDebug("Read " + std::to_string(bytesRead) + " bytes from fd=" + std::to_string(fd) +
                       ", forwarding to fd=" + std::to_string(targetFd));

    ssize_t sent = send(targetFd, buffer, bytesRead, 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        } else {
            m_Logger.logError("Send failed on fd=" + std::to_string(targetFd) + " (" + strerror(errno) + ")");
            onClose(targetFd);
            return;
        }
    }
    else if (sent < bytesRead) {
        m_PendingWrites[targetFd].append(buffer + sent, bytesRead - sent);
    }
}

void Connection::onWritable(int fd) {
    refreshActivity();
    auto it = m_PendingWrites.find(fd);
    if (it == m_PendingWrites.end() || it->second.empty()) {
        return;
    }

    std::string& data = it->second;
    ssize_t sent = send(fd, data.data(), data.size(), 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        } else {
            m_Logger.logError("Send failed on fd=" + std::to_string(fd) + " (" + strerror(errno) + ")");
            onClose(fd);
            return;
        }
    }

    data.erase(0, sent);
    if (data.empty()) {
        m_PendingWrites.erase(it);
    }
}


void Connection::onClose(int fd) {
    m_Logger.logInfo("Close event on fd " + std::to_string(fd));

    if (fd == m_ClientFd) {
        m_Logger.logDebug("Client socket closed");
        close(m_ClientFd);
        m_ClientFd = -1;
    } else if (fd == m_BackendFd) {
        m_Logger.logDebug("Backend socket closed");
        close(m_BackendFd);
        m_BackendFd = -1;
    }

    if (m_ClientFd < 0 && m_BackendFd < 0) {
        m_Logger.logDebug("Both ends closed; cleaning up connection");
        m_Connected = false;
    }
}


void Connection::refreshActivity() {
    m_LastActivity = std::chrono::steady_clock::now();
}

bool Connection::isIdleFor(std::chrono::seconds duration) const {
    auto now = std::chrono::steady_clock::now();
    return (now - m_LastActivity) > duration;
}
