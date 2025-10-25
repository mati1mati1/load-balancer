#include "router.h"

Router::Router(BackendPool& backendPool, RoutingAlgorithm algorithm)
    : m_BackendPool(backendPool), m_Algorithm(algorithm) {}

BackendConfig Router::selectBackend() {
    switch (m_Algorithm) {
        case RoutingAlgorithm::RoundRobin:
            return m_BackendPool.getNextBackend();
        case RoutingAlgorithm::LeastConnections:
            // Implement least connections logic
            break;
        case RoutingAlgorithm::Random:
            // Implement random selection logic
            break;
    }
    throw std::runtime_error("Unknown routing algorithm");
}
