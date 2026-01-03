#include "tracker_connection.h"

static int generate_tracker_key_u32() {
    int key{};
    int ret = RAND_bytes((unsigned char *)&key, sizeof(key));
    if(ret != 1) {
        std::cerr << "RAND_bytes" << std::endl;
        exit(-1);
    }

    return key;
}

TrackerConnection::TrackerConnection(const std::string& host, u16 port, const TorrentFile& file, const std::string& peer_id)
    : m_file{file}, socket{host, port}, m_state{SEND}, m_interval{}, m_peer_id{peer_id}, m_tracker_id{}
{
    last_time = time(0);
}

TrackerConnection::~TrackerConnection()
{}

void TrackerConnection::OnWrite() {
    if(socket.status == CONNECTING) {
        int err{};
        socklen_t len{sizeof(err)};
        getsockopt(socket.fd, SOL_SOCKET, SO_ERROR, &err, &len);

        if(err != 0) {
            return;
        }

        socket.status = CONNECTED;
    }

    time_t now = time(0);
    if(socket.status == CONNECTED && m_state == SEND && difftime(now, last_time) >= m_interval) {
        std::string request = PrepareRequest(m_interval == 0 ? STARTED : NONE);
        socket.SendBuf(request);

        last_time = now;
        m_state = READ;
    }
}

void TrackerConnection::OnRead() {
    if(socket.status == CONNECTING)
        return;

    if(m_state == READ) {
        std::string resp = socket.ReadBuf();
        if(IsValidResponse(resp))
            ParseResponse(resp);

        m_state = SEND;
    }
}

void TrackerConnection::OnError() {
    m_state = ERROR;
}

bool TrackerConnection::IsValidResponse(const std::string& resp) {
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

void TrackerConnection::ParseResponse(const std::string& msg) {
    const std::string delm = "\r\n\r\n";

    auto it = msg.find(delm);
    if(it == std::string::npos) {
        return;
    }

    std::stringstream stream{msg.substr(it + delm.size())};
    bencode::BencodePtr dict_bencode = bencode::ParseElement(stream);
    if(!std::holds_alternative<bencode::Dict>(dict_bencode->val))
        return;

    bencode::Dict *dict = std::get_if<bencode::Dict>(&dict_bencode->val);
    if(!dict) // shouldn't be null since we check before
        return;

    if(auto interval_it = dict->find("interval"); interval_it != dict->end()) {
        assert(std::holds_alternative<i64>(interval_it->second->val));
        m_interval = static_cast<int>(std::get<i64>(interval_it->second->val));
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
            discovered_peers.push_back({str.substr(0, str.size() - 1), port});
        }
    }
}


std::string TrackerConnection::PrepareRequest(Event ev) {
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
        case NONE: // Since this is a regular request at intervals we only send tracker id 
            oss << "&trackerid=" << m_tracker_id;
            event = oss.str();
            break;
    }

    ss << "GET /announce?info_hash=" << url_encode(m_file.GetInfoHash())
    << "&peer_id=" << m_peer_id
    << "&port=" << PORT
    << "&uploaded=" << 0
    << "&downloaded=" << 0
    << "&left=" << 0
    << "&compact=1"
    << event
    << " HTTP/1.1\r\nHost: " << socket.addr
    << "\r\nUser-Agent: LBitTorrent/0.1\r\nConnection: close\r\n\r\n";

    return ss.str();
}

