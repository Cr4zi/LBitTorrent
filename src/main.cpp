#include <iostream>

#include "common.h"
#include "core/torrent_file.h"
#include "net/peer_discovery.h"
#include "net/peers_connection.h"

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
    std::string peer_id = generate_peer_id();
    
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to-file" << std::endl;
        return 1;
    }

    TorrentFile file{argv[1]};
    if(!file.is_file_correct) {
        std::cerr << "Cannot open file" << std::endl;
        return 1;
    }
    
    PeerDiscovery peers_discovery{file};
    const std::vector<Peer>& peers = peers_discovery.GetPeers(peer_id, STARTED);
    for(const Peer& peer : peers) {
        std::cout << peer.ip << ":" << peer.port << std::endl;
    }

    PeersConnection connection{file, peers, peer_id};

    return 0;
}
