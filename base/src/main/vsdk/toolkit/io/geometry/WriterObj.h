#ifndef __WRITER_OBJ__
#define __WRITER_OBJ__

#include "java/io/OutputStream.h"
#include "vsdk/toolkit/io/PersistenceElement.h"

class SimpleScene;
class TriangleMesh;

/**
Writes the triangle meshes of a scene (including the internal meshes of
functional explicit surfaces) to a OBJ stream.
*/
class WriterObj : public PersistenceElement {
public:
    static void exportEnvironment(java::OutputStream& outputStream,
                                  SimpleScene* scene);
};

#endif
