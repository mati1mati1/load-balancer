# 🧠 C++ Load Balancer

A lightweight, high-performance **TCP Load Balancer** written in modern **C++20**, supporting connection pooling, routing strategies, and reactor-based event handling.  
Designed for scalability, extensibility, and clarity — with full **unit test coverage** using **GoogleTest**.

---

## 🚀 Features

### ✅ Core Functionality
- **Reactor Pattern** — efficient event loop for I/O multiplexing (epoll/kqueue abstraction).
- **Acceptor** — handles new client connections asynchronously.
- **Router** — routes clients to backend servers using configurable algorithms:
  - Round Robin
  - Least Connections *(coming soon)*
  - Random *(coming soon)*
- **Backend Pool** — manages backend targets (host/port).
- **Connection Pool** — reuses backend connections to reduce latency.
- **Logger** — structured logging to console or file with log levels.
- **Configuration Manager** — loads JSON config for all system components.

### ⚙️ Advanced Features (Stage 2)
- **Idle Timeout** — closes stale connections automatically.
- **Health Checks** — detect and skip unhealthy backends.
- **Metrics** — track throughput and open connections.
- **Connection Pooling** — reuse backend sockets efficiently.
- **Graceful Shutdown** — drain mode with `drainSeconds`.

---

## 🏗️ Project Structure

```
Load-balancer/
├── include/
│   ├── acceptor.h
│   ├── backend_pool.h
│   ├── connection.h
│   ├── connection_pool.h
│   ├── config_manager.h
│   ├── config_types.h
│   ├── logger.h
│   ├── event_loop_factory.h
│   ├── event_loop.h
│   ├── network_utils.h
│   ├── reactor.h
│   ├── router.h
│   └── interfaces/
│       └── IConnection.h
│       └── ILogger.h
│
├── src/
│   ├── acceptor.cpp
│   ├── backend_pool.cpp
│   ├── connection.cpp
│   ├── connection_pool.cpp
│   ├── epoll_event_loop.cpp
│   ├── kqueue_event_loop.cpp
│   ├── event_loop_factory.cpp
│   ├── network_utils.cpp
│   ├── config_manager.cpp
│   ├── logger.cpp
│   ├── reactor.cpp
│   ├── router.cpp
│   └── main.cpp
│
├── tests/
│   ├── unit/
│   │   ├── acceptor_test.cpp
│   │   ├── backend_pool_test.cpp
│   │   ├── connection_pool_test.cpp
│   │   ├── connection_test.cpp
│   │   ├── reactor_test.cpp
│   │   └── router_test.cpp
│   └── mocks/
│       ├── mock_dependencies.h
│
├── config/
│   └── config.json
│
├── CMakeLists.txt
└── README.md
```

---

## ⚡ Example Configuration

```json
{
  "listen": {
    "host": "127.0.0.1",
    "port": 9000,
    "backlog": 128
  },
  "backends": [
    { "host": "127.0.0.1", "port": 9100 },
    { "host": "127.0.0.1", "port": 9101 }
  ],
  "logging": {
    "level": "info",
    "mode": "stdout",
    "filePath": ""
  },
  "reactor": {
    "threads": 4,
    "connectionReadBuffer": 65536,
    "connectionWriteBuffer": 65536
  },
  "shutdown": {
    "drainSeconds": 10
  }
}
```

---

## 🔁 Architecture Overview

```
                  ┌──────────────────────────┐
                  │        Clients           │
                  └────────────┬─────────────┘
                               │
                     (TCP Accepts via Acceptor)
                               │
                  ┌────────────▼─────────────┐
                  │        Acceptor          │
                  │  - Accepts connections   │
                  │  - Selects backend via   │
                  │    Router                │
                  └────────────┬─────────────┘
                               │
                     (Assign backend)
                               │
                  ┌────────────▼─────────────┐
                  │         Router           │
                  │ - Load balancing logic   │
                  │ - Round Robin / Random   │
                  └────────────┬─────────────┘
                               │
                     (Acquire connection)
                               │
                  ┌────────────▼─────────────┐
                  │     Connection Pool      │
                  │ - Reuse backend sockets  │
                  │ - Handle idle cleanup    │
                  └────────────┬─────────────┘
                               │
                     (Forward to backend)
                               │
                  ┌────────────▼─────────────┐
                  │         Reactor          │
                  │ - Non-blocking I/O loop  │
                  │ - Monitors fds/events    │
                  └────────────┬─────────────┘
                               │
                     (Read / Write Events)
                               │
                  ┌────────────▼─────────────┐
                  │       Connection         │
                  │ - Client ↔ Backend proxy │
                  │ - Detects closure/errors │
                  └──────────────────────────┘
```

---

## 🧪 Building and Running

### **Requirements**
- CMake ≥ 3.16
- GCC ≥ 11 or Clang ≥ 13
- Git
- GoogleTest (auto-fetched)

### **Build**
```bash
git clone https://github.com/<your-user>/cpp-load-balancer.git
cd cpp-load-balancer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

### **Run**
```bash
./build/load_balancer config/config.json
```

---

## 🧰 Run Unit Tests

```bash
cd build
ctest --output-on-failure
```

Or run specific tests:
```bash
./build/connection_pool_test --gtest_filter=ConnectionPoolTest.*
./build/reactor_test --gtest_filter=ReactorTest.*
```

To debug tests:
```bash
lldb ./build/reactor_test -- --gtest_filter=ReactorTest.ClosesIdleConnections
```

---

## 🧱 Design Highlights

| Component | Responsibility |
|------------|----------------|
| `Reactor` | Event loop abstraction; drives I/O readiness |
| `Acceptor` | Accepts new client sockets |
| `Router` | Chooses which backend to forward to |
| `BackendPool` | Manages available backend servers |
| `ConnectionPool` | Caches open backend connections for reuse |
| `Connection` | Forwards data between client and backend |
| `Logger` | Structured logging system |
| `ConfigManager` | Loads and validates configuration |

---

## 🧭 Roadmap

### 🧩 Stage 2 — In Progress
- [x] Connection Pooling  
- [x] Idle Timeout / Auto Close  
- [ ] Health Checks  
- [ ] Load Metrics  
- [ ] Connection Timeouts

### 🧱 Stage 3 — Planned
- [ ] HTTP Layer Support  
- [ ] gRPC load balancing  
- [ ] Web Dashboard for monitoring  
- [ ] Docker + Kubernetes deployment templates

---

## 🧑‍💻 Contributors
- **Matan Amichai** — Core architecture, reactor design, and system implementation.  
- Contributions welcome via pull requests!

---

## 📜 License
MIT License © 2025 — Free for educational and commercial use.

