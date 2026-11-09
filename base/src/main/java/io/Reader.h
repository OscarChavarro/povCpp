#ifndef __READER__
#define __READER__

namespace java {

/**
Minimal stand-in of java.io.Reader: a character source. read() returns the
next char (0..255) or -1 at the end of the stream / on error.
*/
class Reader {
  public:
    virtual int read() = 0;
    virtual void close() = 0;
    virtual ~Reader();
};

}

#endif
