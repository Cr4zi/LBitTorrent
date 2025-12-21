#include <iostream>

#include "common.h"
#include "core/torrent_file.h"
#include "net/peer_discovery.h"

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
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to-file" << std::endl;
        return 1;
    }

    TorrentFile file{argv[1]};
    if(!file.is_file_correct) {
        std::cerr << "Cannot open file" << std::endl;
        return 1;
    }

    PeerDiscovery peers{file};
    peers.GetPeers(generate_peer_id(), STARTED);

    return 0;
}
