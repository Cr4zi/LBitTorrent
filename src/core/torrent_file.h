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

struct TorrentFile {
    bool is_file_correct;
    std::unique_ptr<bencode::Dict> metainfo;

    /* After initializing TorrentFile, should check is_file_correct before moving forward */
    TorrentFile(const std::string& filename);
    ~TorrentFile();

    std::unordered_map<i64, std::vector<std::string>> GetAnnounceList() const;
    std::string GetInfoHash() const;
    std::vector<std::string> GetPieces() const;
    bool IsPrivate() const;
};
