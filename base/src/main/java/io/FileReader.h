#ifndef __FILE_READER__
#define __FILE_READER__

#include "java/io/File.h"
#include "java/io/FileInputStream.h"
#include "java/io/Reader.h"
namespace java {

/**
Reads a file as ISO-8859-1 chars. Unlike the JDK, an unreadable file does not
throw: read() just returns -1 (same convention as FileInputStream).
*/
class FileReader : public Reader {
  private:
    FileInputStream stream;

  public:
    explicit FileReader(const File &file);
    explicit FileReader(const char *path);
    ~FileReader() override;

    int
    read() override;

    void
    close() override;
};

}

#endif
