#include "io/FileLocator.h"

static std::vector<std::string> libraryPaths;

void
FileLocator::clearSearchPaths()
{
    libraryPaths.clear();
}

void
FileLocator::addSearchPath(const char *path)
{
    libraryPaths.push_back(path);
}

const std::vector<std::string> &
FileLocator::searchPaths()
{
    return libraryPaths;
}

FILE *
FileLocator::locate(const char *filename, const char *mode)
{
    FILE *file = fopen(filename, mode);
    if (file != nullptr) {
        return file;
    }

    for (std::vector<std::string>::const_iterator path = libraryPaths.begin();
         path != libraryPaths.end(); ++path) {
        const std::string pathname = *path + "/" + filename;
        file = fopen(pathname.c_str(), mode);
        if (file != nullptr) {
            return file;
        }
    }
    return nullptr;
}
