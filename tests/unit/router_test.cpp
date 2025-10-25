#include <gtest/gtest.h>
#include "router.h"
#include "backend_pool.h"

using namespace std;

class RouterTest : public ::testing::Test {
protected:
    vector<BackendConfig> backends = {
        {"127.0.0.1", 9001},
        {"127.0.0.2", 9002},
        {"127.0.0.3", 9003}
    };
};

// ✅ Test 1: RoundRobin returns correct backend sequence
TEST_F(RouterTest, RoundRobinReturnsBackendsInOrder) {
    BackendPool pool(backends);
    Router router(pool, RoutingAlgorithm::RoundRobin);

    BackendConfig b1 = router.selectBackend();
    BackendConfig b2 = router.selectBackend();
    BackendConfig b3 = router.selectBackend();
    BackendConfig b4 = router.selectBackend(); // should wrap around

    EXPECT_EQ(b1.host, "127.0.0.1");
    EXPECT_EQ(b2.host, "127.0.0.2");
    EXPECT_EQ(b3.host, "127.0.0.3");
    EXPECT_EQ(b4.host, "127.0.0.1"); // ✅ wrap-around
}

// ✅ Test 2: Router holds reference to backend pool
TEST_F(RouterTest, RouterSharesBackendPool) {
    BackendPool pool(backends);
    Router router(pool);

    // modify original pool
    auto& all = const_cast<vector<BackendConfig>&>(pool.getAllBackends());
    all.push_back({"127.0.0.4", 9004});

    // router should now see updated backend
    BackendConfig next = router.selectBackend();
    EXPECT_TRUE(next.host == "127.0.0.1" || next.host == "127.0.0.4" ||
                next.host == "127.0.0.2" || next.host == "127.0.0.3");
}

// ✅ Test 3: LeastConnections algorithm throws (not yet implemented)
TEST_F(RouterTest, LeastConnectionsThrows) {
    BackendPool pool(backends);
    Router router(pool, RoutingAlgorithm::LeastConnections);

    EXPECT_THROW({
        router.selectBackend();
    }, std::runtime_error);
}

// ✅ Test 4: Random algorithm throws (not yet implemented)
TEST_F(RouterTest, RandomThrows) {
    BackendPool pool(backends);
    Router router(pool, RoutingAlgorithm::Random);

    EXPECT_THROW({
        router.selectBackend();
    }, std::runtime_error);
}
