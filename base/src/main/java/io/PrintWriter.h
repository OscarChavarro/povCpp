#ifndef __PRINT_WRITER__
#define __PRINT_WRITER__

#include "java/io/OutputStream.h"
#include "java/lang/String.h"

namespace java {

/**
Emulation of `java.io.PrintWriter` writing text to a byte stream. It
stands for `new PrintWriter(new OutputStreamWriter(out,
StandardCharsets.UTF_8), autoFlush)`: the strings of the port are UTF-8
bytes. As in Java, it never throws: write errors are reported by
`checkError`. The stream is not owned.
*/
class PrintWriter {
  private:
    OutputStream *out;
    bool autoFlush;
    bool error;

  public:
    PrintWriter(OutputStream *out, bool autoFlush);

    void
    print(const java::String &text);

    /**
    Writes the text and a line separator ("\n"), flushing the stream when
    auto flush is enabled.
    */
    void
    println(const java::String &text);

    void
    flush();

    /**
    Flushes the stream.
    @return true if a write failed (i.e. the peer closed the connection)
    */
    bool
    checkError();
};

}

#endif
