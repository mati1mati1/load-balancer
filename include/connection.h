#pragma once
#include "config_types.h"
#include "logger.h"
#include <string>
#include "interfaces/IConnection.h"
class Connection : public IConnection {
public:
    Connection(int clientFd, int backendFd, const BackendConfig& backend, ILogger& logger);
    virtual ~Connection();

    virtual bool connectToBackend() override;
    virtual void closeAll() override;
    virtual void onReadable(int fd) override;
    virtual void onWritable(int fd) override;
    virtual void onClose(int fd) override;
    int getClientFd() const override { return m_ClientFd; }
    int getBackendFd() const override { return m_BackendFd; }
    bool isActive() const noexcept { return m_Connected; }
    bool isConnected() const override { return m_Connected; }
    void setConnected(bool connected) override { m_Connected = connected; }
    const BackendConfig& getBackendConfig() const override { return m_Backend; }
    bool hasBackendOpen() const override { return m_BackendFd >= 0; }
    bool isClientFd(int fd) const override { return fd == m_ClientFd; }
    void refreshActivity();
    virtual bool isIdleFor(std::chrono::seconds duration) const override;
    
private:
    int m_ClientFd;
    int m_BackendFd;
    BackendConfig m_Backend;
    ILogger& m_Logger;
    bool m_Connected;
    std::unordered_map<int, std::string> m_PendingWrites;
    std::chrono::steady_clock::time_point m_LastActivity;
    
};
