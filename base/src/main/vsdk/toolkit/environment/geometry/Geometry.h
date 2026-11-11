#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSupport.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
#include "vsdk/toolkit/environment/geometry/element/RayFirstHitStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
class GeometryBounds;
class RayBoundary;
class RaySolidClassification;
class Ray;
class RayHit;

class Geometry : public Entity {
public:
    static constexpr int INSIDE = static_cast<int>(Containment::INSIDE);
    static constexpr int LIMIT = static_cast<int>(Containment::LIMIT);
    static constexpr int OUTSIDE = static_cast<int>(Containment::OUTSIDE);

    virtual ~Geometry() {}

    // Unsupported is distinct from an empty sequence of crossings.
    virtual RayCrossingSupport emitRayCrossings(
        const Ray&, const RayCrossingQuery&, RayCrossingSink&) const
    { return RayCrossingSupport::Unsupported; }

    // Full solid membership over the query interval; never infer interior from
    // surface hits alone. Unsupported geometries leave result empty.
    virtual RaySolidQueryStatus classifyRay(const Ray&, const RayCrossingQuery&,
        RaySolidClassification& result) const;

    // Inclusive explicit interval; unsupported implementations clear output.
    virtual RayFirstHitStatus queryFirstHit(const Ray&, const RayCrossingQuery&, RayHit*,
        RayBoundary* boundary = nullptr) const;

    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) = 0;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outHit);
    virtual int computeQuantitativeInvisibility(const Vector3Dd& origin, const Vector3Dd& p);
    // Unsupported is explicit; legacy getMinMax remains available.
    virtual void getBounds(GeometryBounds& result) const;
    virtual double* getMinMax() = 0;
    // Legacy containment remains for existing callers; unsupported is explicit here.
    virtual GeometryContainment classifyPoint(const Vector3Dd&, double distanceTolerance) const;
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
};

#endif
