#ifndef __READER_PLY_RESULT__
#define __READER_PLY_RESULT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"

class ReaderPlyResult {
  public:
    TriangleMesh* triangleMesh;
    java::ArrayList<Vector3Dd> pointCloud;

    ReaderPlyResult();
    ~ReaderPlyResult();

    TriangleMesh* detachTriangleMesh();
};

#endif
