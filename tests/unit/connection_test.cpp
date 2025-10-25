#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "connection.h"
#include "logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>     
#include <thread>
#include <chrono>
using namespace std;
using ::testing::_;
using ::testing::HasSubstr;


TEST(ConnectionTest, FailsToConnectInvalidAddress) {
    BackendConfig backend{"256.256.256.256", 9999};  
    Logger logger;


    Connection conn(-1, backend, logger);
    bool result = conn.connectToBackend();

    EXPECT_FALSE(result);
}

TEST(ConnectionTest, FailsToConnectInvalidPort) {
    BackendConfig backend{"127.0.0.1", 9999}; 
    Logger logger;



    Connection conn(-1, backend, logger);
    bool result = conn.connectToBackend();

    EXPECT_FALSE(result);
}

TEST(ConnectionTest, ConnectToValidLocalServer_Forked) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(serverFd, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int rc = ::bind(serverFd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc != 0) {
        int err = errno;
        std::cerr << "bind failed: errno=" << err << " (" << strerror(err) << ")\n";
    }
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(::listen(serverFd, 1), 0);

    pid_t pid = fork();
    ASSERT_GE(pid, 0);

    if (pid == 0) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &len);
        if (clientFd >= 0) {
            close(clientFd);
        }
        close(serverFd);
        _exit(0); 
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); 

        BackendConfig backend{"127.0.0.1", 12345};
        Logger logger;

        Connection conn(-1, backend, logger);
        bool result = conn.connectToBackend();

        EXPECT_TRUE(result);

        close(serverFd);

        int status = 0;
        waitpid(pid, &status, 0);
        EXPECT_TRUE(WIFEXITED(status));
    }
}