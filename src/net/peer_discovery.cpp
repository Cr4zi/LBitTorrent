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


bool PeerDiscovery::valid_response(const std::string& resp) {
    std::istringstream stream{resp};
    std::string line;

    if (!std::getline(stream, line))
        return false;

    std::istringstream status_line{line};
    std::string http_version;
    int status_code;

    status_line >> http_version >> status_code;
    if (status_code != 200)
        return false;

    while (std::getline(stream, line)) {
        if (line == "\r" || line.empty()) // end of headers
            break;

        const std::string key = "Content-Length:";
        if (line.rfind(key, 0) == 0) {
            std::size_t pos = line.find(':');
            size_t content_length = std::stol(line.substr(pos + 1)); // HTTP response cannot be negative
            return content_length != 0;
        }
    }

    return false;
}

std::vector<Peer> PeerDiscovery::parse_resp(const std::string& msg) {
    std::vector<Peer> peers{};

    const std::string delm = "\r\n\r\n";

    auto it = msg.find(delm);
    if(it == std::string::npos) {
        return peers;
    }

    std::stringstream stream{msg.substr(it + delm.size())};
    bencode::BencodePtr dict_bencode = bencode::ParseElement(stream);
    assert(std::holds_alternative<bencode::Dict>(dict_bencode->val));

    bencode::Dict *dict = std::get_if<bencode::Dict>(&dict_bencode->val);
    if(!dict) // shouldn't be null since we check with assert
        return peers;

    if(auto interval_it = dict->find("interval"); interval_it != dict->end()) {
        assert(std::holds_alternative<i64>(interval_it->second->val));
        m_interval = std::get<i64>(interval_it->second->val);
    }

    if(auto id_it = dict->find("tracker id"); id_it != dict->end()) {
        assert(std::holds_alternative<std::string>(id_it->second->val));
        m_tracker_id = std::get<std::string>(id_it->second->val);
    }

    // since I've set compact: 1 we only need to care about peers in binary
    if(auto peers_it = dict->find("peers"); peers_it != dict->end()) { 
        assert(std::holds_alternative<std::string>(peers_it->second->val));

        const std::string& peers_str = std::get<std::string>(peers_it->second->val);
        for(size_t i{}; i < peers_str.size(); i+= 6) {
            std::stringstream ss;

            // since we get IPv4
            for(size_t j{}; j < 4; j++)
                ss << static_cast<unsigned int>(static_cast<unsigned char>(peers_str[i + j])) << '.';

            u16 port = ((static_cast<u16>(peers_str[i + 4]) << 8) | (static_cast<u16>(peers_str[i + 5])));

            const std::string& str = ss.str();
            peers.push_back({str.substr(0, str.size() - 1), port});
        }
    }

    return peers;
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
            resp = pool.Send(messages, 1, valid_response);
        else
            resp = pool.Send(messages, 4, valid_response);
    }

    for(const HostMsg& pair : resp) {
        const std::vector<Peer>& peer = parse_resp(pair.second);
        peers.insert(peers.end(), std::make_move_iterator(peer.begin()), std::make_move_iterator(peer.end()));
    }
    
    return peers;
}
