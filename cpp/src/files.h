#ifndef ICEHALOSIM_FILES_H
#define ICEHALOSIM_FILES_H

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>


namespace IceHalo {
namespace Files {

bool exists(const char* filename);

namespace OpenMode
{
    constexpr uint8_t READ = 0b0001;
    constexpr uint8_t WRITE = 0b0010;
    constexpr uint8_t APPEND = 0b0100;
    constexpr uint8_t BINARY = 0b1000;
};


class File
{
public:
    File(const char* filename);
    ~File();

    bool open(uint8_t mode = OpenMode::READ);
    bool close();

    size_t getSize();

    template<class T>
    size_t read(T* buffer, size_t n = 1);

    template<class T>
    size_t write(T data);

    template<class T>
    size_t write(const T* data, size_t n);

private:
    std::string filename;
    std::FILE *file;
    bool fileOpened;

    boost::filesystem::path p;
    
};


template<class T>
size_t File::read(T* buffer, size_t n)
{
    if (!fileOpened) {
        return 0;
    }

    auto count = std::fread(buffer, sizeof(T), n, file);
    return count;
}


template<class T>
size_t File::write(T data)
{
    if (!fileOpened) {
        return 0;
    }

    auto count = std::fwrite(&data, sizeof(T), 1, file);
    return count;
}


template<class T>
size_t File::write(const T* data, size_t n)
{
    if (!fileOpened) {
        return 0;
    }

    auto count = std::fwrite(data, sizeof(T), n, file);
    return count;
}

}
}


#endif