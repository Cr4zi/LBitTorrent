#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>


// I'm not sure what will be usefull but just in case

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8  = int8_t;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;

constexpr u16 PORT = 6881;

inline std::string url_encode(const std::string& str) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex;
    for(unsigned char ch : str)
        oss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);

    return oss.str();
}
