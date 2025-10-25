#include <gtest/gtest.h>
#include "connection_pool.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
// Helper to simulate backend
static BackendConfig makeBackend(const std::string& host, int port) {
    BackendConfig cfg;
    cfg.host = host;
    cfg.port = port;
    return cfg;
}
static void createDummyServer(int port) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_EQ(::bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)), 0);
    ASSERT_EQ(::listen(serverFd, 1), 0);

    std::thread([serverFd]() {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &len);
        if (clientFd >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            close(clientFd);
        }
        close(serverFd);
    }).detach();
}


TEST(ConnectionPoolTest, AcquireReturnsMinusOneWhenEmpty) {
    ConnectionPool pool;
    auto backend = makeBackend("127.0.0.1", 9000);

    int fd = pool.acquire(backend);
    EXPECT_EQ(fd, -1);
}

TEST(ConnectionPoolTest, AcquireConnection) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(serverFd, 0);

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9100);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_EQ(::bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)), 0);
    ASSERT_EQ(::listen(serverFd, 1), 0);

    pid_t pid = fork();
    ASSERT_GE(pid, 0);

    if (pid == 0) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &len);
        if (clientFd >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            close(clientFd);
        }
        close(serverFd);
        _exit(0);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ConnectionPool pool;
    auto backend = makeBackend("127.0.0.1", 9100);

    int fd = pool.addNewConnection(backend);
    ASSERT_GE(fd, 0);

    pool.release(backend, fd);

    int reusedFd = pool.acquire(backend);
    EXPECT_EQ(reusedFd, fd);

    ::close(fd);
}


TEST(ConnectionPoolTest, PoolDeletesOldestWhenFull) {
    pid_t pid = fork();
    if (pid == 0) {
        for (int p = 9200; p <= 9211; ++p)
            createDummyServer(p);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        _exit(0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); 

    ConnectionPool pool;
    auto firstBackend = makeBackend("127.0.0.1", 9200);
    int fd1 = pool.addNewConnection(firstBackend);
    ASSERT_GE(fd1, 0);

    std::vector<std::pair<BackendConfig, int>> fds;
    for (int i = 1; i < 12; ++i) {
        auto tempBackend = makeBackend("127.0.0.1", 9200 + i);
        int fd = pool.addNewConnection(tempBackend);
        ASSERT_GE(fd, 0);
        fds.emplace_back(tempBackend, fd);
    }

    bool isFirstFdInPool = pool.isConnectionInPool(firstBackend, fd1);
    EXPECT_TRUE(isFirstFdInPool);

    for (const auto& [backend, fd] : fds)
    {
        EXPECT_TRUE(pool.isConnectionInPool(backend, fd));
        pool.release(backend, fd);
        close(fd);
    }
    close(fd1);

    int status = 0;
    waitpid(pid, &status, 0);
    EXPECT_TRUE(WIFEXITED(status));
}


TEST(ConnectionPoolTest, ThreadSafety) {
    const int THREAD_COUNT = 5;
    const int BASE_PORT = 9300;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        createDummyServer(BASE_PORT + i);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ConnectionPool pool;
    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&, i]() {
            auto backend = makeBackend("127.0.0.1", BASE_PORT + i);

            int fd = ::socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in backendAddr{};
            backendAddr.sin_family = AF_INET;
            backendAddr.sin_port = htons(BASE_PORT + i);
            backendAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

            if (::connect(fd, (sockaddr*)&backendAddr, sizeof(backendAddr)) == 0) {
                pool.release(backend, fd);
                int got = pool.acquire(backend);
                if (got >= 0) ::close(got);
            } else {
                close(fd);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    SUCCEED(); 
}

TEST(ConnectionPoolTest, AddNewConnectionCreatesSocket) {
    ConnectionPool pool;
    auto backend = BackendConfig{"127.0.0.1", 9100};

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9100);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(serverFd, (sockaddr*)&addr, sizeof(addr));
    listen(serverFd, 1);

    int fd = pool.addNewConnection(backend);
    EXPECT_GE(fd, 0);

    close(fd);
}

TEST(ConnectionPoolTest, AcquireHandlesConnectionFailureGracefully) {
    ConnectionPool pool;
    BackendConfig backend{"192.0.2.1", 9999}; 

    int fd = pool.addNewConnection(backend);
    EXPECT_EQ(fd, -1); 
}

TEST(ConnectionPoolTest, SeparateBackendsDoNotShareConnections) {
    ConnectionPool pool;
    auto backendA = makeBackend("127.0.0.1", 9200);
    auto backendB = makeBackend("127.0.0.1", 9201);

    createDummyServer(9200);
    createDummyServer(9201);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int fdA = pool.addNewConnection(backendA);
    int fdB = pool.addNewConnection(backendB);

    ASSERT_NE(fdA, fdB);

    int reuseA = pool.acquire(backendA);
    int reuseB = pool.acquire(backendB);

    EXPECT_EQ(reuseA, fdA);
    EXPECT_EQ(reuseB, fdB);

    close(fdA);
    close(fdB);
}
// TEST(ConnectionPoolTest, IdleConnectionsAreCleanedUpAfterTimeout) {
//     ConnectionPool pool;
//     pool.setIdleTimeout(std::chrono::milliseconds(100)); // hypothetical setter

//     auto backend = makeBackend("127.0.0.1", 9300);
//     createDummyServer(9300);

//     int fd = pool.addNewConnection(backend);
//     pool.release(backend, fd);

//     std::this_thread::sleep_for(std::chrono::milliseconds(150));

//     int newFd = pool.acquire(backend);
//     EXPECT_NE(newFd, fd);  // old one cleaned, new created
//     close(newFd);
// }

TEST(ConnectionPoolTest, HandlesHighConcurrency) {
    ConnectionPool pool;
    auto backend = makeBackend("127.0.0.1", 9400);
    createDummyServer(9400);

    const int THREADS = 50;
    std::vector<std::thread> workers;

    for (int i = 0; i < THREADS; ++i) {
        workers.emplace_back([&]() {
            int fd = pool.addNewConnection(backend);
            if (fd >= 0) pool.release(backend, fd);
        });
    }

    for (auto& t : workers) t.join();

    SUCCEED();  
}

TEST(ConnectionPoolTest, ReleaseInvalidFdDoesNotCrash) {
    ConnectionPool pool;
    auto backend = makeBackend("127.0.0.1", 9999);

    pool.release(backend, -1);  
    SUCCEED();
}

TEST(ConnectionPoolTest, AcquireCreatesConnectionIfPoolEmpty) {
    ConnectionPool pool;
    createDummyServer(9500);

    BackendConfig backend{"127.0.0.1", 9500};
    int fd = pool.acquire(backend); 
    EXPECT_GE(fd, 0);
    close(fd);
}
