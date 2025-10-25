#include "network_utils.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

int connectWithTimeout(int fd, const sockaddr_in& addr, int timeoutMs) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int result = ::connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (result == 0) {
        fcntl(fd, F_SETFL, flags);
        return 0;
    }

    if (errno != EINPROGRESS) {
        return -1;
    }

    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    result = select(fd + 1, nullptr, &writefds, nullptr, &tv);
    if (result <= 0) {
        errno = (result == 0) ? ETIMEDOUT : errno;
        return -1;
    }

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0) {
        errno = so_error;
        return -1;
    }

    fcntl(fd, F_SETFL, flags);
    return 0;
}
