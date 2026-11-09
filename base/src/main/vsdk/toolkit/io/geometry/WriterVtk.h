#ifndef __WRITER_VTK__
#define __WRITER_VTK__

#include "java/io/OutputStream.h"
#include "vsdk/toolkit/io/PersistenceElement.h"

class SimpleScene;
class TriangleMesh;

/**
Writes the triangle meshes of a scene (including the internal meshes of
functional explicit surfaces) to a VTK stream.
*/
class WriterVtk : public PersistenceElement {
public:
    static void exportEnvironment(java::OutputStream& outputStream,
                                  SimpleScene* scene);
};

#endif
