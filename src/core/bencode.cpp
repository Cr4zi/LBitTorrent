#include "bencode.h"

std::optional<i64> bencode::ParseInt(std::istream& stream, bool is_str_length) {
    char ch{};
    i64 number{0};
    bool is_negative{false};

    if(!is_str_length) {
        assert(stream.get() == 'i');
        if(stream.peek() == '-') {
            is_negative = true;
            stream.get(); // consume '-'
        }
    }

    while((ch = stream.peek()) != EOF && ch != 'e' && ch != ':') {
        if(!std::isdigit(ch))
            return std::nullopt;

        number = number * 10 + (ch - '0');

        stream.get();
    }

    if(!is_str_length) {
        assert(stream.get() == 'e');
    }

    return is_negative ? -number : number;
}

std::optional<std::vector<u8>> bencode::ParseString(std::istream& stream) {
    std::vector<u8> string{};
    std::optional<i64> length_opt = ParseInt(stream, true);
    if(!length_opt.has_value())
        return std::nullopt;

    assert(stream.get() == ':');
    
    for(i64 i = 0; i < *length_opt; i++) {
        int c = stream.get();

        if(c == EOF)
            return std::nullopt;

        string.push_back(static_cast<u8>(c));
    }

    return string;
}

std::optional<bencode::List> bencode::ParseList(std::istream& stream) {
    List list{};
    char ch{};

    assert(stream.get() == 'l');

    while((ch = stream.peek()) != EOF && ch != 'e') {
        BencodePtr element = ParseElement(stream);
        if(!element)
            return std::nullopt;

        list.push_back(std::move(element));
    }

    assert(stream.get() == 'e');

    return list;
}

std::optional<bencode::Dict> bencode::ParseDict(std::istream& stream) {
    Dict dict{};
    char ch{};

    assert(stream.get() == 'd');

    while((ch = stream.peek()) != EOF && ch != 'e') {
        if(!std::isdigit(ch)) {
            std::cerr << "Invalid dictionary\n";
            return std::nullopt;
        }

        std::optional<std::vector<u8>> key = ParseString(stream);
        if(!key.has_value())
            return std::nullopt;

        BencodePtr value = ParseElement(stream);
        if(!value)
            return std::nullopt;

        dict.emplace(std::move(*key), std::move(value));
    }

    assert(stream.get() == 'e');

    return dict;
}
bencode::BencodePtr bencode::ParseElement(std::istream& file) {
    BencodePtr ptr{std::make_unique<Bencode>()};

    char ch = file.peek();
    switch(ch) {
        case 'i':
            if(auto number = ParseInt(file))
                ptr->val = std::move(*number);
            else {
                std::cerr << "Failed to parse int\n";
                return nullptr;
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(auto string = ParseString(file))
                ptr->val = std::move(*string);
            else {
                std::cerr << "Failed to parse string\n";
                return nullptr;
            }
            break;
        case 'l':
            if(auto list = ParseList(file))
                ptr->val = std::move(*list);
            else {
                std::cerr << "Failed to parse list\n";
                return nullptr;
            }
            break;
        case 'd':
            if(auto dict = ParseDict(file))
                ptr->val = std::move(*dict);
            else {
                std::cerr << "Failed to parse dictionary\n";
                return nullptr;
            }
            break;
        default:
            std::cerr << "Invalid key " << ch << '\n';
            return nullptr;
    }

    return ptr;
}

bencode::BencodePtr bencode::ParseFile(const std::string& filename) {
    BencodePtr ptr;
    std::fstream file{filename, std::ios::in | std::ios::binary};

    if(!file.is_open()) {
        return nullptr;
    }

    char ch{};
    if ((ch = file.peek()) != EOF) {
        ptr = bencode::ParseElement(file);
    }

    file.close();
    return ptr;
}

