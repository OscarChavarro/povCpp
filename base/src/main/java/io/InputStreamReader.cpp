#include "java/io/InputStreamReader.h"
namespace java {

InputStreamReader::InputStreamReader(InputStream &source):
    source(source)
{
}

InputStreamReader::~InputStreamReader() {
}

int
InputStreamReader::read() {
    return source.read();
}

void
InputStreamReader::close() {
    source.close();
}

}
