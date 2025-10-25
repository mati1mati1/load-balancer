#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "reactor.h"
#include "../mocks/mock_dependencies.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Exactly;
using ::testing::Invoke;

TEST(ReactorTest, RegistersConnectionsCorrectly) {
    auto mockLoop = std::make_unique<MockEventLoop>();
    MockLogger logger;
    Reactor reactor(std::move(mockLoop), logger);

    auto conn = std::make_shared<MockConnection>();
    int clientFd = 10;
    int backendFd = 20;
    reactor.registerConnection(conn, clientFd, backendFd);
    auto loopPtr = static_cast<MockEventLoop*>(reactor.getEventLoopForTest());
    EXPECT_CALL(*loopPtr, registerFd(clientFd, true, false)).Times(1);
    EXPECT_CALL(*loopPtr, registerFd(backendFd, true, true)).Times(1);
    EXPECT_CALL(logger, logInfo).Times(1);

    reactor.registerConnection(conn, clientFd, backendFd);
}

TEST(ReactorTest, HandlesReadableEvent) {
    auto mockLoop = std::make_unique<MockEventLoop>();
    MockLogger logger;
    Reactor reactor(std::move(mockLoop), logger);

    auto conn = std::make_shared<MockConnection>();
    int fd = 42;
    reactor.injectConnectionForTest(fd, conn);

    Event e{fd, true, false, false, false};

    EXPECT_CALL(*conn, onReadable(fd)).Times(1);
    EXPECT_CALL(*conn, onWritable(_)).Times(0);
    EXPECT_CALL(*conn, onClose(_)).Times(0);

    reactor.handleEvent(e);
}

TEST(ReactorTest, HandlesWritableEvent) {
    auto mockLoop = std::make_unique<MockEventLoop>();
    MockLogger logger;
    Reactor reactor(std::move(mockLoop), logger);

    auto conn = std::make_shared<MockConnection>();
    int fd = 50;
    reactor.injectConnectionForTest(fd, conn);

    Event e{fd, false, true, false, false};

    EXPECT_CALL(*conn, onWritable(fd)).Times(1);
    EXPECT_CALL(*conn, onReadable(_)).Times(0);

    reactor.handleEvent(e);
}

TEST(ReactorTest, HandlesErrorEventAndUnregisters) {
    auto mockLoop = std::make_unique<MockEventLoop>();
    MockLogger logger;
    Reactor reactor(std::move(mockLoop), logger);

    auto conn = std::make_shared<MockConnection>();
    int fd = 33;
    reactor.injectConnectionForTest(fd, conn);

    EXPECT_CALL(*conn, onClose(fd)).Times(1);
    EXPECT_CALL(logger, logDebug).Times(1);
    EXPECT_CALL(*static_cast<MockEventLoop*>(reactor.getEventLoopForTest()), unregisterFd(fd)).Times(1);

    Event e{fd, false, false, true, false};
    reactor.handleEvent(e);
}

TEST(ReactorTest, StopClosesLoop) {
    auto mockLoop = std::make_unique<MockEventLoop>();
    MockLogger logger;
    auto* loopPtr = mockLoop.get();
    Reactor reactor(std::move(mockLoop), logger);
    reactor.stop();

    EXPECT_CALL(*loopPtr, closeLoop()).Times(1);

}
