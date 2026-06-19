#ifndef __FILE_LOCATOR__
#define __FILE_LOCATOR__

#include <cstdio>

#include "java/io/FileInputStream.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"

class FileLocator {
  private:
    java::ArrayList<java::String> mLibraryPaths;

  public:
    void clearSearchPaths();
    void addSearchPath(const char *path);
    const java::ArrayList<java::String> &searchPaths() const;
    FILE *locate(const char *filename, const char *mode) const;
    java::FileInputStream *locateAsStream(const char *filename) const;
};

#endif
