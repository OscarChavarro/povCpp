#ifndef __WRITER_GTS__
#define __WRITER_GTS__

#include "java/io/OutputStream.h"
#include "vsdk/toolkit/io/PersistenceElement.h"

class SimpleScene;
class TriangleMesh;

/**
Writes the triangle meshes of a scene (including the internal meshes of
functional explicit surfaces) to a GTS stream.
*/
class WriterGts : public PersistenceElement {
public:
    static void exportEnvironment(java::OutputStream& outputStream,
                                  SimpleScene* scene);
};

#endif
