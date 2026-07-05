
#include "render/bakedScene/AabbCullingSupport.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

bool
AabbCullingSupport::pointInsideAabb(const Vector3Dd &point, const AxisAlignedBoundingBox &box, double tolerance)
{
    return
        point.x() >= box.min.x() - tolerance &&
        point.x() <= box.max.x() + tolerance &&
        point.y() >= box.min.y() - tolerance &&
        point.y() <= box.max.y() + tolerance &&
        point.z() >= box.min.z() - tolerance &&
        point.z() <= box.max.z() + tolerance;
}
