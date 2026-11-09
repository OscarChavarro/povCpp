#ifndef __CURVE__
#define __CURVE__

#include "vsdk/toolkit/environment/geometry/Geometry.h"
class Ray;
class RayHit;

class Curve : public Geometry {
public:
    virtual ~Curve() override {}

    virtual Ray* doIntersectionFirstHit(const Ray& r);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) override;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) override;
};

#endif
