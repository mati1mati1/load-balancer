#pragma once
#include "config_types.h"
#include <vector>
#include <atomic>
#include <mutex>

class BackendPool {
public:
    explicit BackendPool(const std::vector<BackendConfig>& backends);

    BackendConfig getNextBackend();

    const std::vector<BackendConfig>& getAllBackends() const;

private:
    std::vector<BackendConfig> m_Backends;
    std::atomic<size_t> m_CurrentIndex{0};
};
