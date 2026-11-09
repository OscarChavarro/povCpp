#ifndef __GEOMETRY_TRIANGULATOR__
#define __GEOMETRY_TRIANGULATOR__

class Geometry;
class TriangleMeshGroup;

class GeometryTriangulator {
public:
    static bool exportToTriangleMeshGroup(Geometry* geometry, TriangleMeshGroup& outGroup);
};

#endif
