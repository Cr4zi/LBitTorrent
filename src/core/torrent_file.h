#pragma once

#include <variant>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <openssl/sha.h>

#include "../common.h"
#include "bencode.h"

struct Tracker {
    std::string host;
    u16 port;

    Tracker(std::string host, u16 port)
        : host{host}, port{port}
    {}

    ~Tracker() {}
};

struct TorrentFile {
    bool is_file_correct;
    std::unique_ptr<bencode::Dict> metainfo;

    /* After initializing TorrentFile, should check is_file_correct before moving forward */
    TorrentFile(const std::string& filename);
    ~TorrentFile();

    std::unordered_map<i64, std::vector<std::string>> GetAnnounceList() const;
    std::string GetInfoHash() const;
    std::vector<std::string> GetPieces() const;
    std::vector<Tracker> GetTrackers() const;
    bool IsPrivate() const;
};
