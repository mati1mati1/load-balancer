#pragma once
#include "config_types.h"
#include "logger.h"
#include <string>
#include "IConnection.h"
class Connection : public IConnection {
public:
    Connection(int clientFd, const BackendConfig& backend, ILogger& logger);
    virtual ~Connection();

    bool connectToBackend();
    void closeAll();
    virtual void onReadable(int fd) override;
    virtual void onWritable(int fd) override;
    virtual void onClose(int fd) override;
    int clientFd() const { return m_ClientFd; }
    int backendFd() const { return m_BackendFd; }
    bool isActive() const noexcept { return m_Connected; }
    bool isConnected() const override { return m_Connected; }
    void setConnected(bool connected) override { m_Connected = connected; }
    int getBackendFd() const override { return m_BackendFd; }
private:
    int m_ClientFd;
    int m_BackendFd;
    BackendConfig m_Backend;
    ILogger& m_Logger;
    bool m_Connected;
    std::unordered_map<int, std::string> m_PendingWrites;
};
