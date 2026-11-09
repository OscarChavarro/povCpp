#include "java/lang/Exception.h"
#include "java/lang/String.h"

#include "java/io/PrintWriter.h"

namespace java {

PrintWriter::PrintWriter(OutputStream *out, bool autoFlush) :
    out(out), autoFlush(autoFlush), error(false)
{
}

void
PrintWriter::print(const java::String &text)
{
    try {
        out->write(reinterpret_cast<const unsigned char *>(text.c_str()), 0,
                   text.length());
    }
    catch ( const java::Exception & ) {
        error = true;
    }
}

void
PrintWriter::println(const java::String &text)
{
    print(text);
    print("\n");
    if ( autoFlush ) {
        flush();
    }
}

void
PrintWriter::flush()
{
    try {
        out->flush();
    }
    catch ( const java::Exception & ) {
        error = true;
    }
}

bool
PrintWriter::checkError()
{
    flush();
    return error;
}

}
