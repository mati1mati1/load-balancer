#pragma once
#include "config_types.h"
#include <unordered_map>
#include <vector>
#include <mutex>

struct PooledBackendConn {
    int fd;
    bool inUse;
    std::chrono::steady_clock::time_point lastUsed;
};

class ConnectionPool {
public:
    ConnectionPool(const ConnectionPoolConfig& config)
        : m_MaxConnectionsPerBackend(config.maxConnectionsPerBackend) {}
    ConnectionPool() : m_MaxConnectionsPerBackend(10) {}
    int acquire(const BackendConfig& backend);
    void release(const BackendConfig& backend, int fd);
    int addNewConnection(const BackendConfig& backend); 
    void cleanupIdleConnections();
    bool isConnectionInPool(const BackendConfig& backend, int fd);

private:
    void removeOldestIdleConnections();
    int CONNECT_TIMEOUT_MS = 3000;
    std::unordered_map<std::string, std::vector<PooledBackendConn>> m_Pool;
    std::mutex m_Mutex;
    const size_t m_MaxConnectionsPerBackend;
};
