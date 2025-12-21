#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <openssl/rand.h>

#include "../common.h"
#include "../core/torrent_file.h"
#include "utils/basic_socket.h"
#include "utils/socket_pool.h"

enum Event {
    STARTED,
    STOPPED,
    COMPLETED
};

struct Peer {
    std::string ip;
    u16 port;
};

class PeerDiscovery {
public:
    const TorrentFile& file;

    PeerDiscovery(const TorrentFile& file);
    ~PeerDiscovery();

    std::vector<Peer> GetPeers(std::string_view peer_id, Event ev);
    
    i64 GetInterval() { return m_interval; }
private:
    i64 m_interval;
    std::string m_tracker_id;

    std::string prepare_request(const std::string& host, std::string_view peer_id, Event ev);
};
