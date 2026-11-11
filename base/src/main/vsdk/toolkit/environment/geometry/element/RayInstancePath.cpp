#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/RayInstancePath.h"
RayInstancePath::RayInstancePath() : surfaceIndex(-1), instances() {}
RayInstancePath::RayInstancePath(const RayInstancePath& other)
    : surfaceIndex(other.surfaceIndex), instances(other.instances) {}
RayInstancePath& RayInstancePath::operator=(const RayInstancePath& other)
{
    if (this != &other) { surfaceIndex = other.surfaceIndex; instances = other.instances; }
    return *this;
}
RayInstancePath::~RayInstancePath() {}
bool RayInstancePath::setSurfaceIndex(long index)
{
    if (index < -1) return false;
    surfaceIndex = index; return true;
}
long RayInstancePath::getSurfaceIndex() const { return surfaceIndex; }
bool RayInstancePath::appendInstance(const RayInstanceStep& step)
{ return step.instance != nullptr && instances.add(step); }
long RayInstancePath::getInstanceCount() const { return instances.size(); }
bool RayInstancePath::getInstance(long index, RayInstanceStep& result) const
{
    if (index < 0 || index >= instances.size()) return false;
    result = instances[index]; return true;
}
void RayInstancePath::clearInstances() { instances.clear(); }
void RayInstancePath::clear() { surfaceIndex = -1; instances.clear(); }
