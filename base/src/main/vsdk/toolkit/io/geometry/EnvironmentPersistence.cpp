#include <cctype>
#include <cstdlib>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/io/geometry/EnvironmentPersistence.h"
#include "vsdk/toolkit/io/geometry/ReaderObj.h"
#include "vsdk/toolkit/io/geometry/ReaderPly.h"
#include "vsdk/toolkit/io/geometry/WriterGts.h"
#include "vsdk/toolkit/io/geometry/WriterObj.h"
#include "vsdk/toolkit/io/geometry/WriterVtk.h"

void
EnvironmentPersistence::importEnvironment(const java::File& sceneFile, SimpleScene* scene)
{
    char* extC = extractExtensionFromFile(sceneFile);
    java::String ext = (extC != 0) ? java::String(extC) : java::String();
    if ( extC != 0 ) free(extC);
    for (size_t i = 0; i < ext.size(); i++) ext[i] = (char)std::tolower((unsigned char)ext[i]);

    if (ext == "obj") {
        ReaderObj::importEnvironment(sceneFile, scene);
        return;
    }
    if (ext == "ply") {
        ReaderPly::importEnvironment(sceneFile, scene);
        return;
    }

    Logger::reportMessage("EnvironmentPersistence", 1,
                          "importEnvironment",
                          "Unsupported scene extension in C++ port");
}

void
EnvironmentPersistence::exportEnvironmentObj(java::OutputStream& outputStream, SimpleScene* scene)
{
    WriterObj::exportEnvironment(outputStream, scene);
}

void
EnvironmentPersistence::exportEnvironmentGts(java::OutputStream& outputStream, SimpleScene* scene)
{
    WriterGts::exportEnvironment(outputStream, scene);
}

void
EnvironmentPersistence::exportEnvironmentVtk(java::OutputStream& outputStream, SimpleScene* scene)
{
    WriterVtk::exportEnvironment(outputStream, scene);
}
