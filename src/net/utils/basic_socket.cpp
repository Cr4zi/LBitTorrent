#include "basic_socket.h"
#include <cerrno>
#include <sys/socket.h>

static bool SetSocketNonBlocking(i32 fd) {
    if(fd < 0)
        return false;

    i32 flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1)
        return false;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

BasicSocket::BasicSocket(const std::string& host, u16 port)
    : is_socket_fine{true},
      addr{host}
{
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    // I don't really like this
    i32 s = getaddrinfo(host.data(), std::to_string(port).c_str(), &hints, &result);
    if(s) {
        std::cerr << "getaddrinfo: " << gai_strerror(s) << std::endl;
        is_socket_fine = false;
        return;
    }

    std::cout << "Trying to initialize: " << host << std::endl;

    struct addrinfo *rp;
    for(rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(fd == -1 || !SetSocketNonBlocking(fd))
            continue;

        if(connect(fd, rp->ai_addr, rp->ai_addrlen) != -1 || errno == EINPROGRESS)
            break;

        close(fd);
    }

    freeaddrinfo(result);

    if(!rp) {
        std::cerr << "Couldn't connect" << std::endl;
        is_socket_fine = false;
    }

    if(errno == EINPROGRESS)
        status = CONNECTING;
    else
        status = CONNECTED;
}

BasicSocket::~BasicSocket() {
    close(fd);
}

void BasicSocket::SendBuf(std::string_view buff) const {
    ssize_t bytes_sent = send(fd, buff.data(), buff.size(), MSG_NOSIGNAL);
    if(bytes_sent == -1) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            std::cerr << "send EWOULDBLOCK or EAGAIN" << std::endl;
        } else {
            std::cerr << "send" << std::endl;
        }
    }
}

std::string BasicSocket::ReadBuf() const {
    std::string result{};
    
    constexpr ssize_t CHUNK = 1024;
    char buf[CHUNK];
    
    ssize_t bytes_read{};
    while((bytes_read = read(fd, buf, CHUNK)) > 0) {
        std::string buff{buf, static_cast<u32>(bytes_read)};
        result.append(buff);
    }

    if(bytes_read == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            std::cerr << "read EAGAIN or EWOULDBLOCK" << std::endl;
        else
            std::cerr << "read" << std::endl;
    }

    return result;
}
