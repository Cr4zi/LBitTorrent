#pragma once

#include <vector>
#include <utility>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <sys/epoll.h>

#include "basic_socket.h"
#include "../../common.h"

using HostMsg = std::pair<const std::string, const std::string>;

class SocketPool {
public:
    bool is_pool_good;
    
    std::vector<std::unique_ptr<BasicSocket>> sockets;

    /* Hold: file descriptor : message */
    std::unordered_map<i32, const std::string> messages;

    SocketPool();
    SocketPool(const std::vector<std::pair<std::string, u16>>& sockets_data);
    ~SocketPool();

    void AddSocket(const std::string& host, u16 port);
    void AddMsg(const std::string& host, const std::string& msg);

    ssize_t Send();
    std::vector<HostMsg> Recv(bool (*valid_response)(const std::string&));
private:
    ssize_t epollfd;
};
