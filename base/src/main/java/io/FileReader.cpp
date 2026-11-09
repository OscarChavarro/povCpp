#include "java/io/FileReader.h"
namespace java {

FileReader::FileReader(const File &file):
    stream(file.getPath().c_str())
{
}

FileReader::FileReader(const char *path):
    stream(path)
{
}

FileReader::~FileReader() {
}

int
FileReader::read() {
    return stream.read();
}

void
FileReader::close() {
    stream.close();
}

}
