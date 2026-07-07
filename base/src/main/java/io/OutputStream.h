#ifndef __OUTPUTSTREAM__
#define __OUTPUTSTREAM__

namespace java {

class OutputStream {
  public:
    virtual void write(int value) = 0;
    virtual void write(const unsigned char *buffer, int offset, int length) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual void dispose() {}
    virtual ~OutputStream() {}
};

}

#endif
