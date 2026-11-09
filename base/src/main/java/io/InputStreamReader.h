#ifndef __INPUT_STREAM_READER__
#define __INPUT_STREAM_READER__

#include "java/io/InputStream.h"
#include "java/io/Reader.h"
namespace java {

/**
Wraps an InputStream converting bytes to chars as ISO-8859-1 (enough for the
ASCII data of Vitral). Deviation from the JDK: the stream is NOT owned, the
caller keeps it alive and destroys it.
*/
class InputStreamReader : public Reader {
  private:
    InputStream &source;

  public:
    explicit InputStreamReader(InputStream &source);
    ~InputStreamReader() override;

    int
    read() override;

    void
    close() override;
};

}

#endif
