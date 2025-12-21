#pragma once

#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <string_view>
#include <errno.h>
#include <string_view>

#include "../../common.h"

enum STATUS {
    CONNECTING,
    CONNECTED,
};

struct BasicSocket {
    i32 fd;
    bool is_socket_fine;
    const std::string addr;
    STATUS status;

    BasicSocket(const std::string& host, u16 port);
    ~BasicSocket();

    void SendBuf(std::string_view buffer) const;
    std::string ReadBuf() const;
};
