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
        /* Send Handshake */

        m_state = HANDSHAKE_READ;
    }
}

void PeerConnection::OnRead() {
    if(socket.status == CONNECTING)
        return;

    if(m_state == HANDSHAKE_READ) {
        /* Parse Handshake */

        m_state = NORMAL;
    }
}

void PeerConnection::OnError() {
    m_state = ERR;
}
