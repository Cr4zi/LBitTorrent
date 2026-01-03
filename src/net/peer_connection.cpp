#include "peer_connection.h"

PeerConnection::PeerConnection(const std::string& host, u16 port, const TorrentFile& file)
    : socket{host, port}, m_state{HANDSHAKE_SEND}, m_file(file)
{}

PeerConnection::~PeerConnection()
{}

void PeerConnection::OnWrite() {
    if(socket.status == CONNECTING) {
        int err{};
        socklen_t len{sizeof(err)};
        getsockopt(socket.fd, SOL_SOCKET, SO_ERROR, &err, &len);

        if(err != 0) {
            return;
        }

        socket.status = CONNECTED;
    }

    if(socket.status == CONNECTED && m_state == HANDSHAKE_SEND) {
        socket.SendBuf(PrepareHandshake());

        m_state = HANDSHAKE_READ;
    } else if(socket.status == CONNECTED && m_state == NORMAL) {
        socket.SendBuf(msg);
    }
}

void PeerConnection::OnRead() {
    if(socket.status == CONNECTING)
        return;

    if(m_state == HANDSHAKE_READ) {
        std::string resp = socket.ReadBuf();

        if(IsValidHandshake(resp))
            m_state = NORMAL;
        else
            m_state = ERR; // if invalid handshake ignore
    } else if(m_state == NORMAL) {
        /* Parse Msg */
    }
}

void PeerConnection::OnError() {
    m_state = ERR;
}

std::string PeerConnection::PrepareHandshake() {
    u8 pstrlen = 19;
    std::string pstr = "BitTorrent protocol";
    char reserved[8] = {0};
    std::string info_hash = m_file.GetInfoHash();

    std::ostringstream oss(std::ios::binary);

    oss.write(reinterpret_cast<const char *>(&pstrlen), 1);
    oss.write(pstr.data(), pstrlen);
    oss.write(reserved, 8);
    oss.write(info_hash.data(), info_hash.size());
    oss.write(m_peer_id.data(), m_peer_id.size());

    return oss.str();
}

bool PeerConnection::IsValidHandshake(const std::string& handshake) {
    // The handshake is a required message and must be the first message transmitted by the client. It is (49+len(pstr)) bytes long.
    // len(pstr) = 19
    if(handshake.size() != 49 + 19)
        return false;

    if(handshake.substr(1, 19) != "BitTorrent protocol")
        return false;

    // check info_hash
    std::string peer_info_hash = handshake.substr(28, 20);
    if (peer_info_hash != m_file.GetInfoHash())
        return false;

    std::string peer_id = handshake.substr(48);
    if(peer_id.size() != 20)
        return false;

    m_peer_id = peer_id;

    return true;
}
