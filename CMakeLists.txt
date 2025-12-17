cmake_minimum_required(VERSION 3.14)

project(LBitTorrent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(CORE src/core/bencode.cpp)

add_executable(${PROJECT_NAME}
    ${CORE}
    src/main.cpp)
