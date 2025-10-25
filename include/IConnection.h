#pragma once
#include "config_types.h"
class IConnection {
public:
    virtual ~IConnection() = default;
    virtual void onReadable(int fd) = 0;
    virtual void onWritable(int fd) = 0;
    virtual void onClose(int fd) = 0;
    virtual bool isConnected() const = 0;
    virtual void setConnected(bool connected) = 0;
    virtual int getBackendFd() const = 0;
    virtual int getClientFd() const = 0;
    virtual bool hasBackendOpen() const = 0;
    virtual bool isClientFd(int fd) const = 0;
    virtual bool connectToBackend() = 0;
    virtual void closeAll() = 0;
    virtual bool isIdleFor(std::chrono::seconds duration) const = 0;
    virtual const BackendConfig& getBackendConfig() const = 0;
};
