#pragma once
#include <gmock/gmock.h>
#include "interfaces/IConnection.h"
#include "event_loop_factory.h"
#include "interfaces/ILogger.h"

class MockEventLoop : public IEventLoop {
public:
    MOCK_METHOD(bool, registerFd, (int fd, bool read, bool write), (override));
    MOCK_METHOD(bool, unregisterFd, (int fd), (override));
    MOCK_METHOD(int, wait, (std::vector<Event>& outEvents, int timeoutMs), (override));
    MOCK_METHOD(void, closeLoop, (), (override));
    MOCK_METHOD(void, updateFd, (int fd, bool wantRead, bool wantWrite), (override));
};

class MockConnection : public IConnection {
public:
    MOCK_METHOD(void, onReadable, (int fd), (override));
    MOCK_METHOD(void, onWritable, (int fd), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(void, setConnected, (bool connected), (override));
    MOCK_METHOD(int, getBackendFd, (), (const, override));
    MOCK_METHOD(int, getClientFd, (), (const, override));
    MOCK_METHOD(bool, hasBackendOpen, (), (const, override));
    MOCK_METHOD(bool, isClientFd, (int fd), (const, override));
    MOCK_METHOD(bool, connectToBackend, (), (override));
    MOCK_METHOD(void, closeAll, (), (override));
    MOCK_METHOD(const BackendConfig&, getBackendConfig, (), (const, override));
    MOCK_METHOD(bool, isIdleFor, (std::chrono::seconds duration), (const, override));
    MOCK_METHOD(void, onClose, (int fd), (override));
private:
    bool m_Closed = false;
 
};

class MockLogger : public ILogger {
public:
    MOCK_METHOD(void, logInfo, (const std::string& msg), ());
    MOCK_METHOD(void, logError, (const std::string& msg), ());
    MOCK_METHOD(void, logDebug, (const std::string& msg), ());
};
