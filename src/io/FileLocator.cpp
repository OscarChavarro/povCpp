#include "io/FileLocator.h"

static java::ArrayList<java::String> libraryPaths;

void
FileLocator::clearSearchPaths()
{
    libraryPaths.clear();
}

void
FileLocator::addSearchPath(const char *path)
{
    libraryPaths.add(java::String(path));
}

const java::ArrayList<java::String> &
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

    for (long int i = 0; i < libraryPaths.size(); i++) {
        char pathname[512];
        snprintf(pathname, sizeof(pathname), "%s/%s", libraryPaths.get(i).toCString(), filename);
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

    for (long int i = 0; i < libraryPaths.size(); i++) {
        char pathname[512];
        snprintf(pathname, sizeof(pathname), "%s/%s", libraryPaths.get(i).toCString(), filename);
        stream = new java::FileInputStream(pathname);
        if (stream->isOpen()) {
            return stream;
        }
        delete stream;
    }
    return nullptr;
}
