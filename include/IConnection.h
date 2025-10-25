#pragma once

class IConnection {
public:
    virtual ~IConnection() = default;
    virtual void onReadable(int fd) = 0;
    virtual void onWritable(int fd) = 0;
    virtual void onClose(int fd) = 0;
    virtual bool isConnected() const = 0;
    virtual void setConnected(bool connected) = 0;
    virtual int getBackendFd() const = 0;
};
