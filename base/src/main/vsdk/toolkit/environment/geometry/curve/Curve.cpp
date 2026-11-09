#include "vsdk/toolkit/environment/geometry/curve/Curve.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
Ray* Curve::doIntersectionFirstHit(const Ray&)
{
    return nullptr;
}

bool Curve::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    Ray* hit = doIntersectionFirstHit(inRay);
    if ( hit == nullptr ) {
        return false;
    }

    if ( outHit != nullptr ) {
        outHit->setRay(*hit);
        doExtraInformation(*hit, hit->getT(), outHit);
    }
    return true;
}

void Curve::doExtraInformation(const Ray&, double, RayHit*)
{
}
