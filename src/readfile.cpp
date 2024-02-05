#include  "readfile.hpp"
#include <fstream>    
std::vector<uint32_t> readFileToUint32Array(const std::string& fileName) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Can't open file");
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size % sizeof(uint32_t) != 0) {
        throw std::runtime_error("The size of file is not a multiple of 4");
    }
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Can't read file");
    }
    return std::vector<uint32_t>(reinterpret_cast<uint32_t*>(buffer.data()), 
                                reinterpret_cast<uint32_t*>(buffer.data() + size));
}
