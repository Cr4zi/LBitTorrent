cmake_minimum_required(VERSION 3.14)

project(LBitTorrent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)

set(CORE src/core/bencode.cpp
    src/core/torrent_file.cpp)

add_executable(${PROJECT_NAME}
    ${CORE}
    src/main.cpp)

target_link_libraries(${PROJECT_NAME}
    OpenSSL::SSL
    OpenSSL::Crypto)
