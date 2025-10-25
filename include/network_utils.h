#pragma once
#include <netinet/in.h>

int connectWithTimeout(int fd, const sockaddr_in& addr, int timeoutMs);
