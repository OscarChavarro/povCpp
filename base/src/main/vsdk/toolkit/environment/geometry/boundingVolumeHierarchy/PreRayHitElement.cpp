#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/PreRayHitElement.h"

PreRayHitElement::~PreRayHitElement()
{
}

bool PreRayHitElement::mayIntersectBefore(const Ray &ray, double maxT) const
{
    (void)maxT;
    return mayIntersect(ray);
}
