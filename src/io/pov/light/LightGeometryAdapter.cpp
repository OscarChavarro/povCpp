#include "io/pov/light/LightGeometryAdapter.h"
#include "vsdk/toolkit/environment/light/PointLight.h"
#include "vsdk/toolkit/environment/light/SpotLight.h"

Light *
LightGeometryAdapter::cloneLight(const Light *light)
{
    if (const SpotLight *spotLight = dynamic_cast<const SpotLight *>(light)) {
        return new SpotLight(*spotLight);
    }
    return new PointLight(*static_cast<const PointLight *>(light));
}

LightGeometryAdapter::LightGeometryAdapter(const LightGeometryAdapter &other) :
    light(other.light != nullptr ? cloneLight(other.light) : nullptr)
{
}

int
LightGeometryAdapter::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    (void)ray;
    (void)depthQueue;
    (void)materialOverride;
    return false;
}

int
LightGeometryAdapter::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    (void)point;
    (void)distanceTolerance;
    return OUTSIDE;
}

void *
LightGeometryAdapter::copy()
{
    return new LightGeometryAdapter(*this);
}
