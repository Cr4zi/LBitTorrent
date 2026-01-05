#include "piece_manager.h"

PieceManager::PieceManager(const TorrentFile& file, std::vector<PeerConnection *>& peers) {
    pieces = file.GetPieces();

    for(size_t i{}; i < pieces.size(); i++) {
        std::vector<PeerConnection *> peers_conn;
        for(PeerConnection *peer : peers) {
            if(peer->bitfield[i])
                peers_conn.push_back(peer);
        }

        total_bitfield.insert({i, {std::move(peers_conn), false}});
    }
}

PieceManager::~PieceManager() {}

void PieceManager::AddPeer(PeerConnection *peer) {
    for(size_t i{}; i < pieces.size(); i++) {
        if(peer->bitfield[i]) {
            std::vector<PeerConnection *>& conns = total_bitfield.at(i).first;
            conns.push_back(peer);
        }
    }
}

void PieceManager::AddPeers(std::vector<PeerConnection *>& peers) {
    for(PeerConnection *peer : peers) {
        AddPeer(peer);
    }
}
