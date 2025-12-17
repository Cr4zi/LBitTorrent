#include <iostream>

#include "core/bencode.h"

int main(int argc, char *argv[]){
    if(argc != 2) {
        std::cerr << "Usage: ./LBitTorrent /path/to-file" << std::endl;
        return 1;
    }

    bencode::BencodePtr parsed = bencode::ParseFile(argv[1]);


    return 0;
}
