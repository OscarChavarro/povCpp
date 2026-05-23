#ifndef __FILE_LOCATOR_H__
#define __FILE_LOCATOR_H__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "java/lang/String.h"
#include "java/io/FileInputStream.h"

class FileLocator {
  public:
    static void clearSearchPaths();
    static void addSearchPath(const char *path);
    static const java::ArrayList<java::String> &searchPaths();
    static FILE *locate(const char *filename, const char *mode);
    static java::FileInputStream *locateAsStream(const char *filename);
};

#endif
