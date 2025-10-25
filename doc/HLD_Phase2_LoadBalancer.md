# Stage 2 HLD — Load Balancer Reliability & Scaling Enhancements

## 1️⃣ Overview

### Objective
Extend the current single-process load balancer into a resilient, observable, and efficient network service that:
- Reuses backend connections to minimize latency.
- Detects unhealthy backends and avoids routing to them.
- Closes idle or stalled sockets to free resources.
- Exposes runtime metrics for monitoring and scaling decisions.

### Scope
Applies to the existing C++ load balancer codebase. This is an architectural enhancement — not a rewrite.

---

## 2️⃣ System Context

```
          ┌──────────────────────┐
          │      Clients         │
          │ (curl, apps, users)  │
          └─────────┬────────────┘
                    │ TCP:9000
           ┌────────▼────────┐
           │  Load Balancer  │
           │────────────────│
           │ - ConnectionMgr │
           │ - Router        │
           │ - Reactor       │
           │ - Logger        │
           │ - Metrics       │
           └─────┬───────────┘
                 │ TCP:9100+
     ┌───────────┼────────────┐
     │           │            │
┌────▼───┐ ┌─────▼───┐ ┌─────▼───┐
│Backend1│ │Backend2 │ │Backend3 │
└────────┘ └─────────┘ └─────────┘
```

---

## 3️⃣ New Components & Responsibilities

| Component | Responsibility |
|------------|----------------|
| **ConnectionPool** | Maintain reusable backend connections for faster routing. |
| **HealthMonitor** | Periodically check backend reachability (TCP connect or heartbeat). |
| **TimeoutManager** | Detect idle or long-blocked sockets and close them. |
| **MetricsCollector** | Track connections, throughput, latency, and health state. |

---

## 4️⃣ Component Designs

### 4.1 Connection Pooling

#### Goal
Reduce backend connection overhead by reusing live connections for future clients.

#### Architecture
- Maintain a pool of ready-to-use backend connections per backend.
- On client connect: Router selects a backend → pool provides existing connection.
- On client disconnect: Return backend connection to pool.

#### Data Model
```cpp
struct PooledConnection {
    int backendFd;
    bool inUse;
    std::chrono::steady_clock::time_point lastUsed;
};

class ConnectionPool {
    std::unordered_map<std::string, std::vector<PooledConnection>> m_Pool;
};
```

#### Policy
- Limit pool size per backend (e.g., 10).
- Drop oldest connections if pool full.
- Idle timeout (e.g., 60 s).

---

### 4.2 Health Checks

#### Goal
Automatically detect and remove unhealthy backends.

#### Design
- Background thread (`HealthMonitor`) runs periodically (every 5 s).
- For each backend: non-blocking connect().
- If success → mark healthy. If fail > N times → mark unhealthy.
- Router skips unhealthy backends.

#### Example
```cpp
class HealthMonitor {
    std::unordered_map<std::string, BackendHealth> m_Status;
    void checkBackend(const BackendConfig& backend);
};

struct BackendHealth {
    bool healthy;
    int failureCount;
    std::chrono::steady_clock::time_point lastChecked;
};
```

---

### 4.3 Timeouts & Idle Detection

#### Goal
Prevent stalled or idle connections from consuming memory and FDs.

#### Implementation
- Each Connection tracks `lastActivity`.
- TimeoutManager closes idle sockets (>60s).

```cpp
class TimeoutManager {
    void checkConnections();
};
```

---

### 4.4 Metrics & Observability

#### Goal
Expose runtime statistics for monitoring, alerting, and scaling.

#### Data Points
| Metric | Description |
|---------|-------------|
| `active_connections_total` | Current open connections |
| `connections_per_backend` | Per-backend load |
| `bytes_transferred_total` | Total data throughput |
| `backend_health_status` | 1 = healthy, 0 = unhealthy |

#### Structure
```cpp
class MetricsCollector {
    std::atomic<int> activeConnections;
    std::atomic<uint64_t> bytesIn, bytesOut;
    std::unordered_map<std::string, BackendStats> perBackend;
};
```

---

## 5️⃣ Concurrency Model

| Thread | Role |
|---------|------|
| Reactor Thread | Handles all I/O events. |
| Health Monitor Thread | Performs backend checks. |
| Timeout Manager Thread | Closes idle/stalled connections. |
| Metrics Reporter Thread | Publishes stats every N seconds. |

---

## 6️⃣ Configuration Extensions

```json
"loadBalancer": {
  "connectionPoolSize": 10,
  "backendCheckInterval": 5,
  "backendFailureThreshold": 3,
  "connectionIdleTimeout": 60,
  "backendConnectTimeout": 5,
  "metricsInterval": 10
}
```

---

## 7️⃣ Error Handling & Recovery

| Scenario | Behavior |
|-----------|-----------|
| Backend down | HealthMonitor marks unhealthy; Router skips. |
| All backends unhealthy | Router returns 503 / TCP RST. |
| Backend timeout | Close FD → retry another backend. |
| Connection pool overflow | Drop oldest idle connection. |

---

## 8️⃣ Testing Strategy

| Test Type | Description |
|------------|-------------|
| Unit Tests | Validate pool reuse, health transitions, timeout closures. |
| Integration Tests | Simulate clients/backends; verify routing & failover. |
| Load Tests | Use `wrk` / `ab` to measure connection reuse. |
| Fault Injection | Kill backend containers to test recovery. |

---

## 🔚 Summary

| Feature | Benefit |
|----------|----------|
| Connection Pooling | Lower latency, fewer `connect()` calls. |
| Health Checks | Automatic failover, resilience. |
| Timeouts | Prevents FD/memory leaks. |
| Metrics | Enables observability & auto-scaling. |
