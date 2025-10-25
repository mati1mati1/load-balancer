#include "connection_pool.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "network_utils.h"

int ConnectionPool::acquire(const BackendConfig& backend) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto& conns = m_Pool[backend.host + ":" + std::to_string(backend.port)];
        for (auto& conn : conns) {
            if (!conn.inUse) {
                conn.inUse = true;
                conn.lastUsed = std::chrono::steady_clock::now();
                return conn.fd;
            }
        }
    }

    return addNewConnection(backend);
}

void ConnectionPool::release(const BackendConfig& backend, int fd) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& conns = m_Pool[backend.host + ":" + std::to_string(backend.port)];

    for (auto& conn : conns) {
        if (conn.fd == fd) {
            conn.inUse = false;
            return;
        }
    }
}

void ConnectionPool::cleanupIdleConnections() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_Pool.begin(); it != m_Pool.end();) {
        auto& conns = it->second;
        conns.erase(std::remove_if(conns.begin(), conns.end(),
            [&](const PooledBackendConn& conn) {
                return !conn.inUse && (now - conn.lastUsed > std::chrono::minutes(5));
            }), conns.end());

        if (conns.empty()) {
            it = m_Pool.erase(it);
        } else {
            ++it;
        }
    }
}

int ConnectionPool::addNewConnection(const BackendConfig& backend) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(backend.port);
    inet_pton(AF_INET, backend.host.c_str(), &addr.sin_addr);

    if (::connectWithTimeout(fd, addr, CONNECT_TIMEOUT_MS) < 0) {
        ::close(fd);
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    std::string key = backend.host + ":" + std::to_string(backend.port);

    if (m_Pool[key].size() < m_MaxConnectionsPerBackend) {
        removeOldestIdleConnections();
        m_Pool[key].push_back({fd, false, std::chrono::steady_clock::now()});
    } else {
        ::close(fd);
        return -1;
    }

    return fd;
}

void ConnectionPool::removeOldestIdleConnections() {
    for (auto& [key, conns] : m_Pool) {
        if (conns.size() <= m_MaxConnectionsPerBackend) continue;

        std::sort(conns.begin(), conns.end(),
            [](const PooledBackendConn& a, const PooledBackendConn& b) {
                return a.lastUsed < b.lastUsed;
            });

        while (conns.size() > m_MaxConnectionsPerBackend) {
            auto it = std::find_if(conns.begin(), conns.end(),
                [](const PooledBackendConn& conn) { return !conn.inUse; });
            if (it != conns.end()) {
                ::close(it->fd);
                conns.erase(it);
            } else {
                break;
            }
        }
    }
}
bool ConnectionPool::isConnectionInPool(const BackendConfig& backend, int fd) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& conns = m_Pool[backend.host + ":" + std::to_string(backend.port)];

    for (const auto& conn : conns) {
        if (conn.fd == fd) {
            return true;
        }
    }
    return false;
}