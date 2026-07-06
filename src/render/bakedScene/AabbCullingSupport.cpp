
#include "render/bakedScene/AabbCullingSupport.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

bool
AabbCullingSupport::pointInsideAabb(const Vector3Dd &point, const AxisAlignedBoundingBox &box, double tolerance)
{
    return
        point.x() >= box.getMin().x() - tolerance &&
        point.x() <= box.getMax().x() + tolerance &&
        point.y() >= box.getMin().y() - tolerance &&
        point.y() <= box.getMax().y() + tolerance &&
        point.z() >= box.getMin().z() - tolerance &&
        point.z() <= box.getMax().z() + tolerance;
}
