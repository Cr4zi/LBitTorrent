#include "peer_discovery.h"
#include <stdexcept>

PeerDiscovery::PeerDiscovery(const TorrentFile& file)
    : file{file}
{}

PeerDiscovery::~PeerDiscovery() {}

int generate_tracker_key_u32() {
    int key{};
    int ret = RAND_bytes((unsigned char *)&key, sizeof(key));
    if(ret != 1) {
        std::cerr << "RAND_bytes" << std::endl;
        exit(-1);
    }

    return key;
}

std::string PeerDiscovery::prepare_request(const std::string& host, std::string_view peer_id, Event ev) {
    std::stringstream ss;

    std::string event{};
    std::ostringstream oss;
    switch(ev) {
        case STARTED:
            oss << "&event=started&key=" << std::hex << std::setw(8) << std::setfill('0') << generate_tracker_key_u32();
            event = oss.str();
            break;
        case STOPPED:
            event = "&event=stopped";
            break;
        case COMPLETED:
            event = "&event=completed";
            break;
    }

    ss << "GET /announce?info_hash=" << url_encode(file.GetInfoHash())
    << "&peer_id=" << peer_id
    << "&port=" << PORT
    << "&uploaded=" << 0
    << "&downloaded=" << 0
    << "&left=" << 0
    << "&compact=1"
    << event
    << " HTTP/1.1\r\nHost: " << host
    << "\r\nUser-Agent: LBitTorrent/0.1\r\nConnection: close\r\n\r\n";

    return ss.str();
}

std::vector<Peer> PeerDiscovery::GetPeers(std::string_view peer_id, Event ev) {
    std::vector<Peer> peers{};

    if(file.IsPrivate()) {
        throw std::runtime_error("Private not implemented yet!");
        return peers;
    }

    std::unordered_map<i64, std::vector<std::string>> announce_map{file.GetAnnounceList()};
    // std::vector<size_t> indicies{announce_map.size(), 0};
    std::vector<HostMsg> resp{};

    while(resp.empty()) {
        std::vector<std::pair<std::string, u16>> sockets_data{};
        std::vector<HostMsg> messages{};

        for(size_t i{}; i < announce_map.size(); i++) {
            /*
            std::cout << announce_map.at(i).size() << std::endl;
            if(indicies.at(i) >= announce_map.at(i).size())
                continue;
                */

            std::string url = announce_map.at(i).at(0);
            ssize_t http_location = url.find("http://");

            if(http_location != 0) {
                //indicies[i]++;
                continue;
            }

            const std::string prefix = "http://";
            const std::string suffix = "/announce";
            
            url = url.substr(prefix.size());

            url = url.substr(0, url.find('/'));

            ssize_t colon = url.find(':');
            
            std::string host = url.substr(0, colon);
            int port = std::stoi(url.substr(colon + 1));
            if(port > USHRT_MAX) // invalid port
                continue;
            
            sockets_data.push_back({host, static_cast<u16>(port)});
            messages.push_back({host, prepare_request(host, peer_id, ev)});

            //indicies[i]++;
        }

        SocketPool pool{std::move(sockets_data)};
        if(ev == STARTED)
            resp = pool.Send(messages, 1);
        else
            resp = pool.Send(messages, 4);
    }

    for(const HostMsg& pair : resp) {
        std::cout << "Host: " << pair.first << "\n Msg: " << pair.second << std::endl;
    }
    
    return peers;
}
