#include <iostream>
#include <sstream>
#include <iomanip>

#include "common.h"
#include "core/torrent_file.h"


int main(int argc, char *argv[]){
    if(argc != 2) {
        std::cerr << "Usage: ./LBitTorrent /path/to-file" << std::endl;
        return 1;
    }

    TorrentFile file{argv[1]};
    if(!file.is_file_correct) {
        std::cerr << "Cannot open file" << std::endl;
        return 1;
    }
    
    for(const std::string& announce : file.GetAnnounceList())
        std::cout << announce << '\n';


    return 0;
}
