#pragma once
#include "backend_pool.h"
#include <string>

enum class RoutingAlgorithm {
    RoundRobin,
    LeastConnections,
    Random
};

class Router {
public:
    explicit Router(BackendPool& backendPool, RoutingAlgorithm algorithm = RoutingAlgorithm::RoundRobin);
    virtual BackendConfig selectBackend();

private:
    BackendPool& m_BackendPool;
    RoutingAlgorithm m_Algorithm;
};
