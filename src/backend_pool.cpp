#include "backend_pool.h"

BackendPool::BackendPool(const std::vector<BackendConfig>& backends)
    : m_Backends(backends) {}

BackendConfig BackendPool::getNextBackend() {
    size_t index = m_CurrentIndex.fetch_add(1, std::memory_order_relaxed);
    return m_Backends[index % m_Backends.size()];
}

const std::vector<BackendConfig>& BackendPool::getAllBackends() const {
    return m_Backends;
}
