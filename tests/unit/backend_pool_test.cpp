#include <gtest/gtest.h>
#include "backend_pool.h"

using namespace std;

class BackendPoolTest : public ::testing::Test {
protected:
    vector<BackendConfig> backends = {
        {"127.0.0.1", 9001},
        {"127.0.0.2", 9002},
        {"127.0.0.3", 9003}
    };
};

// ✅ Test 1: Basic round-robin selection
TEST_F(BackendPoolTest, CyclesThroughBackendsInOrder) {
    BackendPool pool(backends);

    BackendConfig b1 = pool.getNextBackend();
    BackendConfig b2 = pool.getNextBackend();
    BackendConfig b3 = pool.getNextBackend();
    BackendConfig b4 = pool.getNextBackend(); // wraps around

    EXPECT_EQ(b1.host, "127.0.0.1");
    EXPECT_EQ(b2.host, "127.0.0.2");
    EXPECT_EQ(b3.host, "127.0.0.3");
    EXPECT_EQ(b4.host, "127.0.0.1"); // ✅ wraps back
}

// ✅ Test 2: Verify port numbers cycle correctly
TEST_F(BackendPoolTest, CyclesPortsCorrectly) {
    BackendPool pool(backends);

    EXPECT_EQ(pool.getNextBackend().port, 9001);
    EXPECT_EQ(pool.getNextBackend().port, 9002);
    EXPECT_EQ(pool.getNextBackend().port, 9003);
    EXPECT_EQ(pool.getNextBackend().port, 9001); // ✅ wrap
}

// ✅ Test 3: getAllBackends returns correct list
TEST_F(BackendPoolTest, ReturnsAllBackends) {
    BackendPool pool(backends);
    const auto& all = pool.getAllBackends();

    ASSERT_EQ(all.size(), 3);
    EXPECT_EQ(all[0].host, "127.0.0.1");
    EXPECT_EQ(all[1].host, "127.0.0.2");
    EXPECT_EQ(all[2].host, "127.0.0.3");
}

// ✅ Test 4: Works correctly with a single backend
TEST_F(BackendPoolTest, HandlesSingleBackend) {
    vector<BackendConfig> single = {{"127.0.0.1", 9001}};
    BackendPool pool(single);

    for (int i = 0; i < 5; ++i) {
        BackendConfig b = pool.getNextBackend();
        EXPECT_EQ(b.host, "127.0.0.1");
        EXPECT_EQ(b.port, 9001);
    }
}
