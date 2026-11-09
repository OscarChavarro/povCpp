#ifndef __WRITER_COMMON__
#define __WRITER_COMMON__

#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/surface/FunctionalExplicitSurface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"

/**
C++ port helper shared by the scene writers: the triangle mesh to export
for a geometry, or null if it has none.
*/
inline TriangleMesh* writerMeshOf(Geometry* g)
{
    FunctionalExplicitSurface* surface =
        dynamic_cast<FunctionalExplicitSurface*>(g);
    if ( surface != nullptr ) {
        return surface->getInternalTriangleMesh();
    }
    return dynamic_cast<TriangleMesh*>(g);
}

#endif
