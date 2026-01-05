#pragma once

#include <sys/socket.h>
#include <string>

#include "../common.h"
#include "../core/torrent_file.h"
#include "connection.h"
#include "utils/basic_socket.h"

enum PeerState {
    HANDSHAKE_SEND,
    HANDSHAKE_READ,
    NORMAL,
    ERR
};

class PeerConnection : public Connection {
public:
    BasicSocket socket;
    std::string msg;
    std::vector<bool> bitfield;
    
    PeerConnection(const std::string& host, u16 port, const TorrentFile& file);
    ~PeerConnection() override;

    void OnWrite() override;
    void OnRead() override;
    void OnError() override;
private:
    const TorrentFile& m_file;
    PeerState m_state;
    std::string m_peer_id;

    std::string PrepareHandshake();

    /* Will return whether the returned handshake is fine */
    bool IsValidHandshake(const std::string& handshake);
};
