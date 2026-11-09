#include "java/lang/StringBuilder.h"

#include "java/io/BufferedReader.h"

namespace java {

BufferedReader::BufferedReader(InputStream *in) :
    in(in), position(0), count(0), skipLF(false)
{
}

int
BufferedReader::readByte()
{
    if ( position >= count ) {
        count = in->read(buffer, 0, static_cast<int>(sizeof(buffer)));
        position = 0;
        if ( count <= 0 ) {
            count = 0;
            return -1;
        }
    }
    return buffer[position++];
}

bool
BufferedReader::readLine(java::String &outLine)
{
    java::StringBuilder line;
    int c = readByte();

    if ( skipLF && c == '\n' ) {
        c = readByte();
    }
    skipLF = false;
    if ( c < 0 ) {
        return false;
    }
    while ( c >= 0 && c != '\n' && c != '\r' ) {
        line.append(static_cast<char>(c));
        c = readByte();
    }
    // As Java, the "\n" of a "\r\n" is skipped in the next read, so this
    // one does not block waiting for it
    skipLF = c == '\r';
    outLine = line.toString();
    return true;
}

}
