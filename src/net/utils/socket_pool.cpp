#include "socket_pool.h"
#include "basic_socket.h"
#include <utility>


SocketPool::SocketPool()
    : is_pool_good(true), sockets{}, messages{}
{
    epollfd = epoll_create1(0);
    if(epollfd == -1) {
        std::cerr << "Failed creating epoll" << std::endl;
        is_pool_good = false;
        return;
    }
}

SocketPool::SocketPool(const std::vector<std::pair<std::string, u16>>& sockets_data)
    : SocketPool()
{
    if(!is_pool_good)
        return;

    struct epoll_event ev;

    for(const std::pair<const std::string&, u16>& pair : sockets_data) {
        AddSocket(pair.first, pair.second);
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

bool SocketPool::AddSocket(const std::string& host, u16 port) {
    struct epoll_event ev;

    std::unique_ptr<BasicSocket> sock {std::make_unique<BasicSocket>(host, port)};
    if(!sock->is_socket_fine)
        return false;

    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sock->fd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sock->fd, &ev) == -1) {
        std::cerr << "epoll_ctl(add): " << sock->fd << std::endl;
        return false;
    }

    sockets.push_back(std::move(sock));
    return true;
}

bool SocketPool::RemoveSocket(const std::string& host) {
    i32 fd{-1};

    for(auto& socket : sockets) {
        if(socket->addr == host)
            fd = socket->fd;
    }

    if (fd == -1)
        return false;

    if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        std::cerr << "epoll_ctl(del): " << fd << std::endl;
        return false;
    }

    return true;
}

bool SocketPool::AddMsg(const std::string& host, const std::string& msg) {
    i32 fd{-1};

    for(auto& socket : sockets) {
        if(socket->addr == host)
            fd = socket->fd;
    }

    if (fd == -1)
        return false;

    messages.insert({fd, msg});
    return true;
}

ssize_t SocketPool::Send() {
    const i32 MAX_EVENTS = sockets.size() * 2;
    struct epoll_event events[MAX_EVENTS];
    
    ssize_t sent_to{0};
    i32 ndfs{};

    ndfs = epoll_wait(epollfd, events, MAX_EVENTS, -1); // This Block indefinitely

    if(ndfs == -1) {
        std::cerr << "epoll_wait send" << std::endl;
        return -1;
    }

    for(size_t n{}; n < ndfs; n++) {
        i32 fd = events[n].data.fd;
        const auto it = std::find_if(sockets.begin(), sockets.end(), [fd](const std::unique_ptr<BasicSocket>& sock) {
            return sock->fd == fd;
        });

        if(it == sockets.end())
            continue;

        BasicSocket* socket = it->get();

        if(socket->status == CONNECTING && events[n].events & EPOLLOUT) {
            i32 err{};
            socklen_t len {sizeof(err)};
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
            if(err != 0) {
                continue;
            }

            socket->status = CONNECTED;
        }

        if(socket->status == CONNECTED && events[n].events & EPOLLOUT) {
            auto search_message = messages.find(fd);

            if(search_message == messages.end()) {
                // std::cout << "Didn't find?" << socket->addr << std::endl;
                continue;
            }

            socket->SendBuf(search_message->second);
            messages.erase(search_message);
            sent_to++;
        }

    }

    return sent_to;
}

std::vector<HostMsg> SocketPool::Recv(bool (*valid_response)(const std::string&)) {
    const i32 MAX_EVENTS = sockets.size() * 2;
    struct epoll_event events[MAX_EVENTS];
    i32 ndfs{};

    std::vector<HostMsg> result{};

    ndfs = epoll_wait(epollfd, events, MAX_EVENTS, -1);

    if(ndfs == -1) {
        std::cerr << "epoll_wait recv" << std::endl;
        return result;
    }

    for(size_t n{}; n < ndfs; n++) {
        i32 fd = events[n].data.fd;
        const auto it = std::find_if(sockets.begin(), sockets.end(), [fd](const std::unique_ptr<BasicSocket>& sock) {
            return sock->fd == fd;
        });

        if(it == sockets.end())
            continue;

        BasicSocket* socket = it->get();

        if(socket->status == CONNECTED && events[n].events & EPOLLIN) {
            std::string buf = socket->ReadBuf();
            if(buf.size() != 0 && valid_response(buf)) {
                result.emplace_back(socket->addr, buf);
            }

        }
    }

    return result;
}
