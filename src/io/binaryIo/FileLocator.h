#ifndef __FILE_LOCATOR_H__
#define __FILE_LOCATOR_H__

#include <cstdio>

#include "java/io/FileInputStream.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"

class FileLocator {
  private:
    static java::ArrayList<java::String> sLibraryPaths;

  public:
    static void clearSearchPaths();
    static void addSearchPath(const char *path);
    static const java::ArrayList<java::String> &searchPaths();
    static FILE *locate(const char *filename, const char *mode);
    static java::FileInputStream *locateAsStream(const char *filename);
};

#endif
