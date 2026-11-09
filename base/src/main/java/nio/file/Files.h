#ifndef __JAVA_FILES__
#define __JAVA_FILES__

#include "java/lang/String.h"

namespace java {

/**
Static helpers of `java.nio.file.Files` used by the port.
*/
class Files {
  public:
    /**
    Emulation of `Files.readString`: reads a whole text file.
    @param path name of the file
    @param outText the contents of the file
    @return false if the file could not be read (Java throws IOException)
    */
    static bool readString(const String &path, String &outText);

  private:
    Files();
};

}

#endif
