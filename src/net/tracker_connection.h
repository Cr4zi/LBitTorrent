#pragma once

#include <vector>
#include <time.h>
#include <openssl/rand.h>

#include "../common.h"
#include "../core/bencode.h"
#include "../core/torrent_file.h"
#include "connection.h"
#include "utils/basic_socket.h"

struct Peer {
    std::string host;
    u16 port;
};

enum TrackerState {
    SEND,
    READ,
    ERROR
};

enum Event {
    STARTED,
    STOPPED,
    COMPLETED,
    NONE
};

class TrackerConnection : public Connection {
public:
    BasicSocket socket;
    std::vector<Peer> discovered_peers;

    TrackerConnection(const std::string& host, u16 port, const TorrentFile& file, const std::string& peer_id);
    ~TrackerConnection() override;

    void OnWrite() override;
    void OnRead() override;
    void OnError() override;

private:
    const TorrentFile& m_file;
    
    TrackerState m_state;

    int m_interval;
    time_t last_time;

    const std::string& m_peer_id;
    std::string m_tracker_id;

    bool IsValidResponse(const std::string& resp);
    void ParseResponse(const std::string& msg);
    std::string PrepareRequest(Event ev);
};
