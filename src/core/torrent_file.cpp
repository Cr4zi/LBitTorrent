#include "torrent_file.h"
#include "bencode.h"
#include <openssl/sha.h>
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

std::vector<std::string> TorrentFile::GetAnnounceList() {
    std::vector<std::string> announce_list{};

    if(auto search = metainfo->find("announce"); search != metainfo->end()) {
        assert(std::holds_alternative<std::string>(search->second->val));
        
        std::string str = std::get<std::string>(search->second->val);
        announce_list.push_back(str);
    }

    if(auto search = metainfo->find("announce-list"); search != metainfo->end()) {
        assert(std::holds_alternative<bencode::List>(search->second->val));

        bencode::List *list = std::get_if<bencode::List>(&search->second->val);
        if(!list) // shouldn't be null since we did assert
            return announce_list;

        for(auto& announce : *list) {
            assert(std::holds_alternative<bencode::List>(announce->val));

            // Who tf thought this is a good idea?
            bencode::List *list_of_lists = std::get_if<bencode::List>(&announce->val);

            if(!list_of_lists)
                continue;

            // std::cout << bencode::EncodeElement(announce.get()) << std::endl;

            for(auto& url : *list_of_lists) {
                assert(std::holds_alternative<std::string>(url->val));

                announce_list.push_back(std::get<std::string>(url->val));
            }

        }
    }

    return announce_list;
}


std::string TorrentFile::GetInfoHash() {
    if(auto search = metainfo->find("info"); search != metainfo->end()) {
        unsigned char obuf[SHA_DIGEST_LENGTH];
        std::string encoded = bencode::EncodeElement(search->second.get());
        unsigned char *ibuf = reinterpret_cast<unsigned char *>(encoded.data());
        SHA1(ibuf, encoded.size(), obuf);

        return std::string(reinterpret_cast<char *>(obuf), SHA_DIGEST_LENGTH);
    }

    return "";
}
