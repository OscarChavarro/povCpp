#ifndef __FILE_LOCATOR_H__
#define __FILE_LOCATOR_H__

#include <cstdio>
#include <string>
#include <vector>

class FileLocator {
  public:
    static void clearSearchPaths();
    static void addSearchPath(const char *path);
    static const std::vector<std::string> &searchPaths();
    static FILE *locate(const char *filename, const char *mode);
};

#endif
