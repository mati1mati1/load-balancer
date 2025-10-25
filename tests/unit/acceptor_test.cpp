#include <gtest/gtest.h>
#include <thread>
#include <arpa/inet.h>
#include "acceptor.h"
#include "backend_pool.h"
#include "router.h"
#include "logger.h"

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

    auto onAccept = [](int, const BackendConfig&) {};

    EXPECT_THROW({
        Acceptor acceptor(cfg, router, logger, onAccept);
    }, std::runtime_error);
}

TEST(AcceptorTest, AcceptsAndCallsCallback) {
    ListenConfig cfg{"127.0.0.1", 9099, 10};
    vector<BackendConfig> backends = {{"127.0.0.1", 9001}};
    BackendPool pool(backends);
    MockRouter router(pool);
    Logger logger;

    bool callbackCalled = false;
    auto onAccept = [&](int fd, const BackendConfig& backend) {
        callbackCalled = true;
        EXPECT_GE(fd, 0);
        EXPECT_EQ(backend.port, 9001);
        close(fd);
    };

    Acceptor acceptor(cfg, router, logger, onAccept);
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

    bool callbackCalled = false;
    auto onAccept = [&](int fd, const BackendConfig&) {
        callbackCalled = true;
        close(fd);
    };

    Acceptor acceptor(cfg, router, logger, onAccept);
    acceptor.start();
    acceptor.stop();

    EXPECT_FALSE(acceptor.isRunning()); 
}
