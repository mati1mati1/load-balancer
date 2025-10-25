#include <gtest/gtest.h>
#include <thread>
#include <arpa/inet.h>
#include "acceptor.h"
#include "backend_pool.h"
#include "router.h"
#include "logger.h"
#include "connection_pool.h"
using namespace std;


class MockRouter : public Router {
public:
    explicit MockRouter(BackendPool& pool)
        : Router(pool, RoutingAlgorithm::RoundRobin) {}

    BackendConfig selectBackend() override {
        called = true;
        return {"127.0.0.1", 9001};
    }

    bool called = false;
};


TEST(AcceptorTest, ThrowsIfBindFails) {
    ListenConfig cfg{"127.0.0.1", 80, 10}; 
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;

    auto onAccept = [](std::shared_ptr<IConnection> conn, int, const BackendConfig&) {};
    ConnectionPool connectionPool;
    EXPECT_THROW({
        Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    }, std::runtime_error);
}

TEST(AcceptorTest, AcceptsAndCallsCallback) {
    ListenConfig cfg{"127.0.0.1", 9099, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;

    bool callbackCalled = false;
    auto onAccept = [&](std::shared_ptr<IConnection> conn, int, const BackendConfig& backend) {
        callbackCalled = true;
        EXPECT_GE(conn->getBackendFd(), -1);
        EXPECT_EQ(backend.port, 9001);
        conn->closeAll();
    };
    
    Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    acceptor.start();

    int clientFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.port);
    addr.sin_addr.s_addr = inet_addr(cfg.host.c_str());
    connect(clientFd, (struct sockaddr*)&addr, sizeof(addr));
    close(clientFd);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    acceptor.stop();

    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(router.called);
}

TEST(AcceptorTest, StopCleansUpProperly) {
    ListenConfig cfg{"127.0.0.1", 9100, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;
    
    bool callbackCalled = false;
    auto onAccept = [&](std::shared_ptr<IConnection> conn, int, const BackendConfig&) {
        callbackCalled = true;
        conn->closeAll();
    };

    Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    acceptor.start();
    acceptor.stop();

    EXPECT_FALSE(acceptor.isRunning()); 
}

TEST(AcceptorTest, StartStopMultipleTimesIsSafe) {
    ListenConfig cfg{"127.0.0.1", 9110, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;

    bool callbackCalled = false;
    auto onAccept = [&](std::shared_ptr<IConnection> conn, int, const BackendConfig&) {
        callbackCalled = true;
        conn->closeAll();
    };

    Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);

    acceptor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    acceptor.stop();

    acceptor.stop();

    acceptor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    acceptor.stop();

    EXPECT_FALSE(acceptor.isRunning());
}

TEST(AcceptorTest, InvalidHostThrows) {
    ListenConfig cfg{"invalid_host_name", 9200, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;

    auto onAccept = [](std::shared_ptr<IConnection>, int, const BackendConfig&) {};

    EXPECT_THROW({
        Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    }, std::runtime_error);
}

TEST(AcceptorTest, HandlesMultipleClients) {
    ListenConfig cfg{"127.0.0.1", 9300, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;

    std::atomic<int> callbackCount{0};
    auto onAccept = [&](std::shared_ptr<IConnection> conn, int, const BackendConfig&) {
        callbackCount++;
        conn->closeAll();
    };

    Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    acceptor.start();

    for (int i = 0; i < 5; ++i) {
        int clientFd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(cfg.port);
        addr.sin_addr.s_addr = inet_addr(cfg.host.c_str());
        connect(clientFd, (struct sockaddr*)&addr, sizeof(addr));
        close(clientFd);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    acceptor.stop();

    EXPECT_GE(callbackCount.load(), 1);
}

TEST(AcceptorTest, BindToUsedPortThrows) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9400);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ASSERT_EQ(::bind(serverFd, (sockaddr*)&addr, sizeof(addr)), 0);
    ASSERT_EQ(::listen(serverFd, 1), 0);

    ListenConfig cfg{"127.0.0.1", 9400, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;

    auto onAccept = [](std::shared_ptr<IConnection>, int, const BackendConfig&) {};

    EXPECT_THROW({
        Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    }, std::runtime_error);

    close(serverFd);
}

TEST(AcceptorTest, ClientDisconnectTriggersCleanup) {
    ListenConfig cfg{"127.0.0.1", 9500, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;

    std::atomic<bool> callbackCalled = false;
    auto onAccept = [&](std::shared_ptr<IConnection> conn, int, const BackendConfig&) {
        callbackCalled = true;
        conn->closeAll();
    };

    Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    acceptor.start();

    int clientFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.port);
    addr.sin_addr.s_addr = inet_addr(cfg.host.c_str());
    connect(clientFd, (struct sockaddr*)&addr, sizeof(addr));
    close(clientFd); 

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    acceptor.stop();

    EXPECT_TRUE(callbackCalled.load());
}

TEST(AcceptorTest, LoggerLogsStartup) {
    ListenConfig cfg{"127.0.0.1", 9600, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;
    ConnectionPool connectionPool;

    auto onAccept = [](std::shared_ptr<IConnection>, int, const BackendConfig&) {};

    Acceptor acceptor(cfg, router, logger, connectionPool, onAccept);
    acceptor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    acceptor.stop();

    SUCCEED(); 
}

