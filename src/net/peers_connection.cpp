#include "peers_connection.h"


PeersConnection::PeersConnection(const TorrentFile& torrent_file, std::vector<Peer> peers, const std::string& peer_id)
    : is_connection_good{true}, m_torrent_file{torrent_file},  m_full_peers{}, m_peer_id{peer_id}, m_pool{}
{
    PerformHandshake(peers);
    
    for(const FullPeer& peer : m_full_peers) {
        std::cout << "IP: " << peer.ip << " PORT: " << peer.port << " ID: " << peer.peer_id << std::endl;
    }
}

PeersConnection::~PeersConnection() {}

void PeersConnection::PerformHandshake(const std::vector<Peer>& peers) {
    const std::string handshake = PrepareHandshake();
    
    for(const Peer& peer : peers) {
        if(m_pool.AddSocket(peer.ip, peer.port)) {
            m_pool.AddMsg(peer.ip, handshake);
        }
    }

    while(m_pool.Send() == 0);

    std::vector<HostMsg> responses = m_pool.Recv([](const std::string& msg) {return msg.size() > 1;});
    while(responses.size() == 0)
        responses = m_pool.Recv([](const std::string& msg) {return msg.size() > 1;});

    for (const HostMsg& resp : responses) {
        std::cout << resp.second << std::endl;
        ParseHandshake(resp, peers);
    }
}

std::string PeersConnection::PrepareHandshake() {
    u8 pstrlen = 19;
    std::string pstr = "BitTorrent protocol";
    u64 reserved = 0;
    std::string info_hash = m_torrent_file.GetInfoHash();

    std::ostringstream oss(std::ios::binary);

    oss.write(reinterpret_cast<const char *>(&pstrlen), 1);
    oss.write(pstr.data(), pstrlen);
    oss.write(reinterpret_cast<const char *>(&reserved), 8);
    oss.write(info_hash.data(), info_hash.size());
    oss.write(m_peer_id.data(), m_peer_id.size());

    return oss.str();
}

void PeersConnection::ParseHandshake(const HostMsg& resp, const std::vector<Peer>& peers) {
    /* 1 - pstrlen is 1 byte
     * 19 - pstr
     * 8 - reserved
     * 1 - cuz inclusive */
    std::string hash_and_id = resp.second.substr(1 + 19 + 8 + 1);
    const std::string& host = resp.first;

    std::cout << "Here sigma" << std::endl;
    
    if(hash_and_id.substr(20) != m_torrent_file.GetInfoHash()) {
        m_pool.RemoveSocket(host);
        return;
    }

    const auto peer_it = std::find_if(peers.begin(), peers.end(), [host](const Peer& peer){
        return peer.ip == host;
    });

    if(peer_it == peers.end()) {
        std::cerr << "No peer " << host << std::endl;
        return;
    }

    FullPeer peer{*peer_it, hash_and_id.substr(21)};
    
    m_full_peers.insert(peer);
}
