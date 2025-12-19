#pragma once

#include <memory>
#include <variant>
#include <vector>
#include <map>
#include <string_view>
#include <fstream>
#include <optional>
#include <cctype>
#include <iostream>
#include <cassert>

#include "../common.h"

namespace bencode {
    struct Bencode;
    using BencodePtr = std::unique_ptr<Bencode>;

    using Dict = std::map<std::vector<u8>, BencodePtr>;
    using List = std::vector<BencodePtr>;
    
    struct Bencode {
        std::variant<i64, std::vector<u8>, List, Dict> val;
    };

    std::optional<i64> ParseInt(std::istream& stream, bool is_str_length=false);
    std::optional<std::vector<u8>> ParseString(std::istream& stream);
    std::optional<List> ParseList(std::istream& stream);
    std::optional<Dict> ParseDict(std::istream& stream);
    BencodePtr ParseElement(std::istream& file);

    // I am doing const std:string& because there is no constructor for std::fstream(std::string_view)
    // and if I'm already creating a copy for that string I may as well use it for the whole
    // function
    BencodePtr ParseFile(const std::string& filename);

    std::string EncodeElement(Bencode* element);
    std::string EncodeElement(const std::vector<u8>& vec);
}
