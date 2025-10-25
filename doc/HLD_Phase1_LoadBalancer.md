# 🧠 High-Level Design (HLD) — C++ Load Balancer (Phase 1)

## 1. Overview
The **C++ Load Balancer** is a lightweight, high-performance proxy designed to distribute incoming client traffic among multiple backend servers.  
The goal of **Phase 1** is to build a **minimal but functional TCP load balancer** with **Round Robin** routing and **multithreaded** connection handling.  
This phase focuses on learning and implementing the fundamentals of networking, concurrency, and design patterns in C++.

---

## 2. Objectives
- Accept incoming client TCP connections.
- Forward data transparently between client and backend.
- Implement **Round Robin** routing.
- Handle **multiple concurrent connections**.
- Add basic **logging** (connection start/end/errors).
- Load configuration from a **JSON file**.
- Implement **graceful shutdown**.

---

## 3. Scope (Phase 1)
**Included**
- TCP proxy (no HTTP parsing).
- Static backend list (from config file).
- Round Robin routing.
- Multithreaded connection handling.
- Structured logging.

**Excluded (for future phases)**
- Health checks.
- Dynamic backend registration.
- TLS termination.
- Advanced routing algorithms.
- Prometheus-style metrics.

---

## 4. System Architecture

### 4.1 Logical Components
| Component | Responsibility |
|------------|----------------|
| **ConfigManager** | Loads configuration (listen port, backends, settings) from JSON file. |
| **BackendPool** | Manages backend servers and Round Robin selection. |
| **Router** | Chooses the next backend using the current algorithm. |
| **Acceptor** | Listens for incoming client connections. |
| **Connection** | Handles data transfer between a client and its backend. |
| **Reactor** | Manages I/O events using epoll/select and dispatches to connections. |
| **Logger** | Writes structured logs (e.g., JSON or text). |
| **ShutdownManager** | Handles graceful shutdown when SIGINT/SIGTERM are received. |

---

## 5. Data Flow (High Level)
1. **Client** connects to the load balancer.
2. **Acceptor** accepts the connection and assigns it to a worker thread.
3. **Router** selects the next backend from **BackendPool** (Round Robin).
4. A **Connection** object is created for the client and backend pair.
5. Data is forwarded **bidirectionally** between client and backend sockets.
6. When either side closes the connection, the **Connection** object cleans up.
7. **Logger** records lifecycle events and errors.
8. On shutdown, **ShutdownManager** stops accepting new clients and waits for active connections to finish.

---

## 6. Threading Model
- A **main thread** starts the load balancer and spawns:
  - 1 **Acceptor thread** (handles `accept()` calls).
  - N **Worker threads** (handle client/backend I/O using epoll).
- The number of worker threads is configurable (default = CPU core count).
- Each connection’s I/O is processed by a single worker thread to avoid race conditions.

---

## 7. Configuration
Configuration is stored in a file named `config.json`.

**Example:**
```json
{
  "listen": {
    "host": "0.0.0.0",
    "port": 8080
  },
  "backends": [
    { "host": "127.0.0.1", "port": 9001 },
    { "host": "127.0.0.1", "port": 9002 }
  ],
  "threads": 4,
  "logging": {
    "level": "info",
    "mode": "stdout"
  }
}
```

---

## 8. Key Algorithms

### 8.1 Round Robin Routing
Each new connection is assigned to the next backend in sequence:
```
backendIndex = (backendIndex + 1) % backendCount
```

### 8.2 Non-blocking I/O
- Sockets are set to **non-blocking mode**.
- **epoll** is used to monitor read/write events efficiently.
- Prevents blocking threads when waiting for I/O.

### 8.3 Backpressure Handling
When one side (client or backend) writes faster than the other can read:
- Temporarily disable `EPOLLIN` on the faster side.
- Resume when buffers are drained.

### 8.4 Graceful Shutdown
- Stop accepting new connections.
- Allow existing connections to complete (timeout configurable).
- Close remaining sockets and release resources.

---

## 9. Logging Design

**Log Format (JSON-like):**
```json
{
  "timestamp": "2025-10-17T15:03:25Z",
  "level": "info",
  "event": "connection_open",
  "client": "192.168.0.10:51234",
  "backend": "127.0.0.1:9001",
  "connectionId": "abcd1234"
}
```

**Log Types:**
| Type | Description |
|------|--------------|
| `connection_open` | A new client connection was accepted. |
| `connection_close` | The connection was closed normally or due to error. |
| `error` | Any socket or system-level error. |
| `shutdown` | Triggered when the system begins shutting down. |

---

## 10. Error Handling
- **Accept errors:** log and continue.
- **Connect failures:** log and close client socket.
- **Read/write errors:** log and close connection.
- **Invalid configuration:** print error and exit.

---

## 11. Class Diagram (Simplified)

```
 ┌──────────────────┐
 │ ConfigManager    │
 │ - loadConfig()   │
 │ - getBackends()  │
 └────────┬─────────┘
          │
 ┌────────▼──────────┐
 │ BackendPool       │
 │ - selectNext()    │
 │ - addBackend()    │
 └────────┬──────────┘
          │
 ┌────────▼─────────┐
 │ Router           │
 │ - getBackend()   │
 └────────┬─────────┘
          │
 ┌────────▼─────────┐
 │ Acceptor         │
 │ - start()        │
 │ - stop()         │
 └────────┬─────────┘
          │
 ┌────────▼─────────────┐
 │ Connection           │
 │ - handleRead()       │
 │ - handleWrite()      │
 │ - close()            │
 └────────┬─────────────┘
          │
 ┌────────▼─────────────┐
 │ Reactor              │
 │ - registerFd()       │
 │ - pollEvents()       │
 └──────────────────────┘
```

---

## 12. Testing Plan

### Unit Tests
- ✅ ConfigManager correctly loads valid/invalid JSON.
- ✅ BackendPool wraps correctly on Round Robin.
- ✅ Logger formats valid JSON log lines.

### Integration Tests
- Start two simple echo servers as backends.
- Send multiple client connections and verify even distribution.
- Test large data transfers (e.g., 100MB) to verify stability.

### Stress Tests
- Run 10,000 concurrent clients using `wrk` or a custom TCP load generator.
- Observe CPU/memory usage.
- Verify graceful shutdown behavior.

---

## 13. Future Enhancements
| Feature | Description |
|----------|-------------|
| Health Checks | Periodically ping backends and remove unhealthy ones. |
| Weighted Routing | Assign weights to backends for uneven load distribution. |
| Configuration Reload | Allow reloading config without restarting. |
| Metrics | Export connection stats via HTTP endpoint. |
| TLS Termination | Accept HTTPS connections and forward decrypted traffic. |
| Sticky Sessions | Route clients consistently to the same backend. |

---

## 14. Directory Structure

```
cpp-load-balancer/
│
├── src/
│   ├── main.cpp
│   ├── config_manager.cpp
│   ├── backend_pool.cpp
│   ├── acceptor.cpp
│   ├── connection.cpp
│   ├── reactor.cpp
│   ├── router.cpp
│   └── logger.cpp
│
├── include/
│   ├── config_manager.h
│   ├── backend_pool.h
│   ├── acceptor.h
│   ├── connection.h
│   ├── reactor.h
│   ├── router.h
│   └── logger.h
│
├── config/
│   └── config.json
│
├── tests/
│   ├── unit/
│   └── integration/
│
└── docs/
    └── HLD_Phase1_LoadBalancer.md
```

---

## 15. Success Criteria
✅ The load balancer:
1. Accepts connections on the configured port.  
2. Routes traffic evenly between backends.  
3. Handles many simultaneous clients without blocking.  
4. Logs key events and errors.  
5. Shuts down gracefully.  
