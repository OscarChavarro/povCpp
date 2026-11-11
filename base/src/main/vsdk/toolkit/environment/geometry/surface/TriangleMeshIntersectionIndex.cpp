#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshIntersectionIndex.h"

TriangleMeshIntersectionIndex::TriangleMeshIntersectionIndex() : value(-1) {}
TriangleMeshIntersectionIndex::TriangleMeshIntersectionIndex(const TriangleMeshIntersectionIndex& other) :
    value(other.get()) {}
TriangleMeshIntersectionIndex& TriangleMeshIntersectionIndex::operator=(const TriangleMeshIntersectionIndex& other)
{
    if (this != &other) value.set(other.get());
    return *this;
}
int TriangleMeshIntersectionIndex::get() const { return (int)value.get(); }
void TriangleMeshIntersectionIndex::set(int index) { value.set(index); }
