#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "torrent_file.h"
#include "../net/peer_connection.h"

struct PieceManager {
    std::vector<std::string> pieces;

    // <piece_index, <list of peers with that piece, is_downloaded>
    std::unordered_map<size_t, std::pair<std::vector<PeerConnection *>, bool>> total_bitfield;

    PieceManager(const TorrentFile& file, std::vector<PeerConnection *>& peers);
    ~PieceManager();

    void AddPeer(PeerConnection *peer);
    void AddPeers(std::vector<PeerConnection *>& peers);

    void HandoutPieces();
    void UpdateBitfields();
};
