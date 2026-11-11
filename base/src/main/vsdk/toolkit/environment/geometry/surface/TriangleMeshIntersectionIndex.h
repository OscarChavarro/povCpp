#ifndef VSDK_TRIANGLE_MESH_INTERSECTION_INDEX_H
#define VSDK_TRIANGLE_MESH_INTERSECTION_INDEX_H

#include "java/util/concurrent/atomic/AtomicLong.h"

// Legacy last-hit index; queries carrying their own index must not depend on it.
class TriangleMeshIntersectionIndex {
private:
    java::AtomicLong value;
public:
    TriangleMeshIntersectionIndex();
    TriangleMeshIntersectionIndex(const TriangleMeshIntersectionIndex& other);
    TriangleMeshIntersectionIndex& operator=(const TriangleMeshIntersectionIndex& other);
    int get() const;
    void set(int index);
};
#endif
