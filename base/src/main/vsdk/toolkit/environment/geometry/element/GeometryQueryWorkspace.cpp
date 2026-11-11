#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/GeometryQueryWorkspace.h"
GeometryQueryWorkspace::GeometryQueryWorkspace() : solid(), bounds(), hit(), scratch(), scratchDepth(0) {}
GeometryQueryWorkspace::~GeometryQueryWorkspace()
{
    for (long i = 0; i < scratch.size(); i++) delete scratch[i];
}
RaySolidClassification& GeometryQueryWorkspace::solidResult() { return solid; }
GeometryBounds& GeometryQueryWorkspace::boundsResult() { return bounds; }
RayHit& GeometryQueryWorkspace::hitResult() { return hit; }
RaySolidClassification* GeometryQueryWorkspace::acquireSolidScratch()
{
    if (scratchDepth == scratch.size()) {
        RaySolidClassification* value = new RaySolidClassification();
        try {
            if (!scratch.add(value)) { delete value; return nullptr; }
        }
        catch (...) {
            delete value;
            throw;
        }
    }
    RaySolidClassification* result = scratch[scratchDepth++];
    result->clear();
    return result;
}
bool GeometryQueryWorkspace::releaseSolidScratch(RaySolidClassification* value)
{
    if (scratchDepth == 0 || value != scratch[scratchDepth - 1]) return false;
    value->clear();
    scratchDepth--;
    return true;
}
void GeometryQueryWorkspace::clear()
{
    solid.clear(); bounds.setUnsupported(); hit.clear();
    for (long i = 0; i < scratchDepth; i++) scratch[i]->clear();
    scratchDepth = 0;
}
