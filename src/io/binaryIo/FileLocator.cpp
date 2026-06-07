#include "io/binaryIo/FileLocator.h"

java::ArrayList<java::String> FileLocator::sLibraryPaths;

void
FileLocator::clearSearchPaths()
{
    sLibraryPaths.clear();
}

void
FileLocator::addSearchPath(const char *path)
{
    sLibraryPaths.add(java::String(path));
}

const java::ArrayList<java::String> &
FileLocator::searchPaths()
{
    return sLibraryPaths;
}

FILE *
FileLocator::locate(const char *filename, const char *mode)
{
    FILE *file = fopen(filename, mode);
    if (file != nullptr) {
        return file;
    }

    for (long int i = 0; i < sLibraryPaths.size(); i++) {
        char pathname[512];
        snprintf(pathname, sizeof(pathname), "%s/%s", sLibraryPaths.get(i).toCString(), filename);
        file = fopen(pathname, mode);
        if (file != nullptr) {
            return file;
        }
    }
    return nullptr;
}

java::FileInputStream *
FileLocator::locateAsStream(const char *filename)
{
    java::FileInputStream *stream = new java::FileInputStream(filename);
    if (stream->isOpen()) {
        return stream;
    }
    delete stream;

    for (long int i = 0; i < sLibraryPaths.size(); i++) {
        char pathname[512];
        snprintf(pathname, sizeof(pathname), "%s/%s", sLibraryPaths.get(i).toCString(), filename);
        stream = new java::FileInputStream(pathname);
        if (stream->isOpen()) {
            return stream;
        }
        delete stream;
    }
    return nullptr;
}
