#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <functional>
#include <unordered_set>
#include <algorithm>

#include "../common.h"
#include "../core/torrent_file.h"
#include "peer_discovery.h"
#include "utils/socket_pool.h"

struct FullPeer {
    std::string ip;
    u16 port;

    std::string peer_id;
    std::vector<u8> bitfield;

    bool am_choking, am_interested, peer_choking, peer_interested;

    explicit FullPeer(const Peer& peer, std::string peer_id)
        : ip(peer.ip), port(peer.port), am_choking(true), am_interested(false), peer_choking(true), peer_interested(false),
          peer_id(peer_id)
    {}

    ~FullPeer() {}

    bool operator==(const FullPeer& other) const {
        return ip == other.ip && port == other.port;
    }
};

namespace std {
    template<>
    struct hash<FullPeer> {
        inline size_t operator()(const FullPeer& full_peer) const {
            return std::hash<std::string>{}(full_peer.ip);
        }
    };
}

class PeersConnection {
public:
    bool is_connection_good;
    
    PeersConnection(const TorrentFile& torrent_file, std::vector<Peer> peers, const std::string& peer_id);
    ~PeersConnection();

    void PerformHandshake(const std::vector<Peer>& peers);
private:
    const TorrentFile& m_torrent_file;
    std::unordered_set<FullPeer> m_full_peers;
    const std::string& m_peer_id;
    SocketPool m_pool;

    std::string PrepareHandshake();

    void ParseHandshake(const HostMsg& resp, const std::vector<Peer>& peers);
};
