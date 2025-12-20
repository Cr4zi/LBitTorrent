#include "torrent_file.h"
#include "bencode.h"
#include <variant>

TorrentFile::TorrentFile(const std::string& filename)
    : is_file_correct{true}
{
    /* 8 is the length of .torrent if it's not obvious */
    if(filename.substr(filename.size() - 8) != ".torrent") {
        is_file_correct = false;
        return;
    }

    bencode::BencodePtr file = bencode::ParseFile(filename);
    if(bencode::Dict* dict = std::get_if<bencode::Dict>(&file->val)) {
        metainfo = std::make_unique<bencode::Dict>(std::move(*dict));
        return;
    } 
    is_file_correct = false;
}

TorrentFile::~TorrentFile() {}

std::unordered_map<i64, std::vector<std::string>> TorrentFile::GetAnnounceList() {
    std::unordered_map<i64, std::vector<std::string>> announce_map{};

    if(auto search = metainfo->find("announce-list"); search != metainfo->end()) {
        assert(std::holds_alternative<bencode::List>(search->second->val));

        bencode::List *list = std::get_if<bencode::List>(&search->second->val);
        if(!list) // shouldn't be null since we did assert
            return announce_map;

        i64 tier = 0;
        for(auto& announce : *list) {
            assert(std::holds_alternative<bencode::List>(announce->val));

            // Who tf thought this is a good idea?
            bencode::List *list_of_lists = std::get_if<bencode::List>(&announce->val);

            if(!list_of_lists)
                continue;

            std::vector<std::string> announce_list{};

            for(auto& url : *list_of_lists) {
                assert(std::holds_alternative<std::string>(url->val));

                announce_list.push_back(std::get<std::string>(url->val));
            }

            announce_map.insert({tier, std::move(announce_list)});
            tier++;
        }
    } else if(auto search = metainfo->find("announce"); search != metainfo->end()) {
        assert(std::holds_alternative<std::string>(search->second->val));

        std::string str = std::get<std::string>(search->second->val);
        std::vector<std::string> vec{str};
        announce_map.insert({0, vec});
    }

    return announce_map;
}


std::string TorrentFile::GetInfoHash() {
    if(auto search = metainfo->find("info"); search != metainfo->end()) {
        assert(std::holds_alternative<bencode::Dict>(search->second->val));
        
        unsigned char obuf[SHA_DIGEST_LENGTH];
        std::string encoded = bencode::EncodeElement(search->second.get());
        unsigned char *ibuf = reinterpret_cast<unsigned char *>(encoded.data());
        SHA1(ibuf, encoded.size(), obuf);

        return std::string(reinterpret_cast<char *>(obuf), SHA_DIGEST_LENGTH);
    }

    return "";
}

std::vector<std::string> TorrentFile::GetPieces() {
    std::vector<std::string> pieces{};
    if(auto search = metainfo->find("info"); search != metainfo->end()) {
        assert(std::holds_alternative<bencode::Dict>(search->second->val));
        
        bencode::Dict* info = std::get_if<bencode::Dict>(&search->second->val);
        if(!info)
            return pieces;
        
        if(auto search_pieces = info->find("pieces"); search_pieces != info->end()) {
            assert(std::holds_alternative<std::string>(search_pieces->second->val));
            
            std::string pieces_str = std::get<std::string>(search_pieces->second->val);
            for(size_t i{}; i < pieces_str.length() / 20; i++) {
                pieces.push_back(pieces_str.substr(i * 20, (i + 1) * 20));
            }
        }
    }

    return pieces;
}

bool TorrentFile::IsPrivate() {
    if(auto search = metainfo->find("info"); search != metainfo->end()) {
        assert(std::holds_alternative<bencode::Dict>(search->second->val));

        bencode::Dict* info = std::get_if<bencode::Dict>(&search->second->val);
        if(!info)
            return false;

        if(auto private_search = info->find("private"); private_search != info->end()) {
            assert(std::holds_alternative<i64>(private_search->second->val));
            return std::get<i64>(private_search->second->val) >= 1 ? true : false;
        }
    }

    return false;
}
