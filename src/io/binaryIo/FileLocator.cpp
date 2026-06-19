#include "io/binaryIo/FileLocator.h"

void
FileLocator::clearSearchPaths()
{
    mLibraryPaths.clear();
}

void
FileLocator::addSearchPath(const char *path)
{
    mLibraryPaths.add(java::String(path));
}

const java::ArrayList<java::String> &
FileLocator::searchPaths() const
{
    return mLibraryPaths;
}

FILE *
FileLocator::locate(const char *filename, const char *mode) const
{
    FILE *file = fopen(filename, mode);
    if (file != nullptr) {
        return file;
    }

    for (long int i = 0; i < mLibraryPaths.size(); i++) {
        char pathname[512];
        snprintf(pathname, sizeof(pathname), "%s/%s", mLibraryPaths.get(i).toCString(), filename);
        file = fopen(pathname, mode);
        if (file != nullptr) {
            return file;
        }
    }
    return nullptr;
}

java::FileInputStream *
FileLocator::locateAsStream(const char *filename) const
{
    const java::File file(filename);
    if (file.canRead()) {
        return new java::FileInputStream(filename);
    }

    for (long int i = 0; i < mLibraryPaths.size(); i++) {
        char pathname[512];
        snprintf(pathname, sizeof(pathname), "%s/%s", mLibraryPaths.get(i).toCString(), filename);
        const java::File candidate(pathname);
        if (candidate.canRead()) {
            return new java::FileInputStream(pathname);
        }
    }
    return nullptr;
}
