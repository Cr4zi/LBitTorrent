#pragma once

#include <vector>
#include <utility>
#include <string_view>
#include <memory>
#include <algorithm>
#include <sys/epoll.h>

#include "basic_socket.h"
#include "../../common.h"

using HostMsg = std::pair<const std::string, const std::string>;

class SocketPool {
public:
    bool is_pool_good;
    
    std::vector<std::unique_ptr<BasicSocket>> sockets;

    SocketPool(const std::vector<std::pair<std::string, u16>>& sockets_data);
    ~SocketPool();

    /* @param messages Vector of pair<host, message>
     * @param when_to_stop after what amount stop reading
     * @returns Vector of pair<host, response> */
    std::vector<HostMsg> Send(const std::vector<HostMsg>& messages, i32 when_to_stop);
private:
    ssize_t epollfd;
};
