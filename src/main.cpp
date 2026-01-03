#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <sys/epoll.h>
#include <openssl/rand.h>

#include "common.h"
#include "core/torrent_file.h"
#include "net/connection.h"
#include "net/tracker_connection.h"
#include "net/peer_connection.h"
#include "net/utils/basic_socket.h"

#define ARGC_RETURN_CODE 1
#define TORRENT_FILE_RETURN_CODE 2
#define EMPTY_TRACKERS_RETURN_CODE 3
#define EPOLL_CREATE_RETURN_CODE 4
#define EPOLL_CTL_RETURN_CODE 5
#define EPOLL_WAIT_RETURN_CODE 6

#define MAX_EVENTS 128

std::string generate_peer_id() {
    std::ostringstream oss;
    oss << "-LB0001-";
    unsigned char buf[12];
    int ret = RAND_bytes(buf, sizeof(buf));
    if(ret != 1) {
        std::cerr << "RAND_bytes" << std::endl;
        exit(-1);
    }

    char digits[] = "0123456789";
    size_t len = 10;
    for(int i = 0; i < 12; i++)
        oss << digits[buf[i] % len];

    return oss.str();
}

int main(int argc, char *argv[]){
    struct epoll_event events[MAX_EVENTS];
    std::string peer_id = generate_peer_id();
    std::unordered_map<int, std::unique_ptr<Connection>> connections{};

    // We we'll use this set to check whether we get the same peer twice. If we get twice we'll not add to epoll
    std::unordered_set<std::string> peers_hosts;
    
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to-file" << std::endl;
        return ARGC_RETURN_CODE;
    }

    TorrentFile file{argv[1]};
    if(!file.is_file_correct) {
        std::cerr << "Cannot open file" << std::endl;
        return TORRENT_FILE_RETURN_CODE;
    }
    
    std::vector<Tracker> trackers = file.GetTrackers();
    if(trackers.empty()) {
        std::cerr << "Empty trackers" << std::endl;
        return EMPTY_TRACKERS_RETURN_CODE;
    }
    
    int epollfd = epoll_create1(0);
    if(epollfd == -1) {
        std::cerr << "epoll_create1" << std::endl;
        return EPOLL_CREATE_RETURN_CODE;
    }

    for(const Tracker& tracker : trackers) {
        struct epoll_event ev{};
        std::unique_ptr<TrackerConnection> conn = std::make_unique<TrackerConnection>(tracker.host, tracker.port, file, peer_id);
        if(!conn->socket.is_socket_fine)
            continue;
        
        int fd = conn->socket.fd;

        // We first only want to send tracker requests
        ev.events = EPOLLOUT | EPOLLIN;
        ev.data.ptr = conn.get();

        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, conn->socket.fd, &ev) == -1) {
            std::cerr << "epoll_ctl: tracker socket" << std::endl;
            return EPOLL_CTL_RETURN_CODE;
        }

        connections.insert({fd, std::move(conn)});
    }

    for(;;) {
        /* 50 ms delay */
        int ndfs = epoll_wait(epollfd, events, MAX_EVENTS, 0);
        if(ndfs == -1) {
            std::cerr << "epoll_wait" << std::endl;
            return EPOLL_WAIT_RETURN_CODE;
        }

        for(int n{}; n < ndfs; n++) {
            Connection *conn = static_cast<Connection *>(events[n].data.ptr);

            if(events[n].events & EPOLLERR) {
                conn->OnError();
            }

            if(events[n].events & EPOLLOUT) {
                conn->OnWrite();
            }

            if(events[n].events & EPOLLIN) {
                conn->OnRead();

                const TrackerConnection *tracker = dynamic_cast<const TrackerConnection *>(conn);
                if(tracker != nullptr) {
                    for(const Peer& peer: tracker->discovered_peers) {
                        if(auto it = peers_hosts.find(peer.host); it != peers_hosts.end())
                            continue;

                        struct epoll_event ev{};

                        std::unique_ptr<PeerConnection> conn = std::make_unique<PeerConnection>(peer.host, peer.port, file);
                        if(!conn->socket.is_socket_fine)
                            continue;
                        
                        int fd = conn->socket.fd;

                        ev.events = EPOLLOUT | EPOLLIN;
                        ev.data.ptr = conn.get();

                        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, conn->socket.fd, &ev) == -1) {
                            std::cerr << "epoll_ctl: peer socket" << std::endl;
                            return EPOLL_CTL_RETURN_CODE;
                        }
                        std::cout << "Added peer: " << peer.host << ":" << peer.port << std::endl;

                        connections.insert({fd, std::move(conn)});
                        peers_hosts.insert(peer.host);
                        
                    }
                }
            }
            
        }
    }

    return 0;
}
