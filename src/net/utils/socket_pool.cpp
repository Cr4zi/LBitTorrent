#include "socket_pool.h"
#include "basic_socket.h"
#include <utility>

SocketPool::SocketPool(const std::vector<std::pair<std::string, u16>>& sockets_data)
    : is_pool_good{true}
{
    epollfd = epoll_create1(0);
    if(epollfd == -1) {
        std::cerr << "Failed creating epoll" << std::endl;
        is_pool_good = false;
        return;
    }

    struct epoll_event ev;

    for(const std::pair<const std::string&, u16>& pair : sockets_data) {
        std::unique_ptr<BasicSocket> sock {std::make_unique<BasicSocket>(pair.first, pair.second)};
        if(!sock->is_socket_fine)
            continue;

        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = sock->fd;
        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sock->fd, &ev) == -1) {
            std::cerr << "epoll_ctl(add): " << sock->fd << std::endl;
            continue;
        }
        sockets.push_back(std::move(sock));
    }

    if(sockets.size() < 1) {
        std::cerr << "Initializing sockets failed" << std::endl;
        is_pool_good = false;
        return;
    }
}

// I'm pretty sure the destructor of sockets goes over every BasicSocket and calls the destructor
SocketPool::~SocketPool() {
    close(epollfd);
}

std::vector<HostMsg> SocketPool::Send(const std::vector<HostMsg>& messages, i32 when_to_stop, bool (*valid_response)(const std::string&)) {
    const i32 MAX_EVENTS = sockets.size() * 2;
    struct epoll_event events[MAX_EVENTS];
    std::vector<HostMsg> result{};
    i32 read_from{};
    i32 ndfs{};

    while(read_from < when_to_stop) {
        ndfs = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        if(ndfs == -1) {
            std::cerr << "epoll_wait" << std::endl;
            return result;
        }

        for(size_t n{}; n < ndfs; n++) {
            int fd = events[n].data.fd;
            const auto it = std::find_if(sockets.begin(), sockets.end(), [fd](const std::unique_ptr<BasicSocket>& sock) {
                return sock->fd == fd;
            });

            if(it == sockets.end())
                continue;

            BasicSocket* socket = it->get();

            if(socket->status == CONNECTING && events[n].events & EPOLLOUT) {
                int err{};
                socklen_t len {sizeof(err)};
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if(err != 0) {
                    continue;
                }

                socket->status = CONNECTED;
            }

            if(socket->status == CONNECTED && events[n].events & EPOLLOUT) {
                
                const auto host_it = std::find_if(messages.begin(), messages.end(), [socket](const HostMsg& host_msg) {
                    return socket->addr == host_msg.first;
                });

                if(host_it == messages.end()) {
                    // std::cout << "Didn't find?" << socket->addr << std::endl;
                    continue;
                }

                socket->SendBuf(host_it->second);
            } else if(socket->status == CONNECTED && events[n].events & EPOLLIN) {
                std::string buf = socket->ReadBuf();
                if(buf.size() != 0 && valid_response(buf)) {
                    result.emplace_back(socket->addr, buf);
                    read_from++;
                }
            }
            
        }
    }

    return result;
}
