#ifndef __BUFFERED_READER__
#define __BUFFERED_READER__

#include "java/io/InputStream.h"
#include "java/lang/String.h"

namespace java {

/**
Emulation of `java.io.BufferedReader` reading lines of text from a byte
stream. It stands for `new BufferedReader(new InputStreamReader(in,
StandardCharsets.UTF_8))`: the strings of the port are UTF-8 bytes, so no
decoding is needed. The stream is not owned.
*/
class BufferedReader {
  private:
    InputStream *in;
    unsigned char buffer[4096];
    int position;
    int count;
    /// A line ended by "\r": a "\n" following it is not a new line
    bool skipLF;

    int
    readByte();

  public:
    explicit BufferedReader(InputStream *in);

    /**
    Reads a line, ended by "\n", "\r" or "\r\n" (not included in it).
    @param outLine the line read
    @return false at the end of the stream, when Java returns null
    */
    bool
    readLine(java::String &outLine);
};

}

#endif
