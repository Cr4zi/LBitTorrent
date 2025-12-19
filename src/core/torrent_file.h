#pragma once

#include <variant>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <openssl/sha.h>

#include "bencode.h"

struct TorrentFile {
    bool is_file_correct;
    std::unique_ptr<bencode::Dict> metainfo;

    /* After initializing TorrentFile, should check is_file_correct before moving forward */
    TorrentFile(const std::string& filename);
    ~TorrentFile();

    std::vector<std::string> GetAnnounceList();
    std::string GetInfoHash();
};
