#ifndef VSDK_READER_MIT_TRIANGLE_GROUP_H
#define VSDK_READER_MIT_TRIANGLE_GROUP_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
class SimpleMaterial;
class SimpleScene;

// One MIT triangles block; material changes flush faces while keeping vertices.
class ReaderMitTriangleGroup {
private:
    java::ArrayList<Vector3Dd> vertices;
    java::ArrayList<Triangle> triangles;
public:
    ReaderMitTriangleGroup();
    ~ReaderMitTriangleGroup();
    void reset();
    bool addVertex(const Vector3Dd& vertex);
    bool addFace(const char* coordinates);
    void flush(SimpleScene* scene, const SimpleMaterial* material);
};
#endif
