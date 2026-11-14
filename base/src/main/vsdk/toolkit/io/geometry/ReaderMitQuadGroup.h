#ifndef VSDK_READER_MIT_QUAD_GROUP_H
#define VSDK_READER_MIT_QUAD_GROUP_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class SimpleBody;
class SimpleMaterial;
class SimpleScene;

// One MIT quads block: `v x y z` vertices and `q i0 i1 i2 i3` faces, built as a
// QuadMesh; material changes flush faces while keeping vertices.
class ReaderMitQuadGroup {
private:
    java::ArrayList<Vector3Dd> vertices;
    java::ArrayList<int> quadIndices;
public:
    ReaderMitQuadGroup();
    ~ReaderMitQuadGroup();
    void reset();
    bool addVertex(const Vector3Dd& vertex);
    // Reads four vertex indices; false when malformed or out of range.
    bool addQuad(const char* indices);
    // Adds the pending faces as one body and returns it, or null when none are pending.
    SimpleBody* flush(SimpleScene* scene, const SimpleMaterial* material);
};
#endif
