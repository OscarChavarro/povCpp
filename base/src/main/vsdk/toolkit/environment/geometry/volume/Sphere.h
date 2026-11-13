#ifndef __SPHERE__
#define __SPHERE__

#include <cmath>
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class Ray;
class RayHit;
class PolyhedralBoundedSolid;

class Sphere : public Solid {
private:
    double radius;
    double radiusSquared;

    static const int DEFAULT_PARALLELS = 8;
    static const int DEFAULT_MERIDIANS = 16;
    static const int MIN_PARALLELS = 2;
    static const int MIN_MERIDIANS = 3;

    static Vector3Dd spherePosition(double theta, double t, double r);
    bool validateSurfaceQuery(const Ray& ray, const RayCrossingQuery& query,
        RayFirstHitStatus& failure) const;
    void fillSurfaceDetail(const Ray& ray, double t, RayHit* result) const;
    PolyhedralBoundedSolid* buildPolyhedralBoundedSolid(int nmeridians, int nparalels);

public:
    explicit Sphere(double r);
    Geometry* copyGeometry() const override { return new Sphere(radius); }
    RayCrossingSupport emitRayCrossings(const Ray& ray,
        const RayCrossingQuery& query, RayCrossingSink& sink) const override;

    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;

    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* result, RayBoundary* boundary = nullptr) const override;

    // Returns zero, one or two roots in increasing order without allocation.
    static int intersectionRoots(const Vector3Dd& origin, const Vector3Dd& direction,
        double radius, const RayCrossingQuery& query, double& nearT, double& farT);

    virtual ~Sphere() {}

    Ray* doIntersectionFirstHit(const Ray& inoutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    // Explicit inclusive interval; the legacy overload queries [0,+infinity).
    bool doIntersectionFirstHit(const Ray& inRay, const RayCrossingQuery& query,
        RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);
    GeometryContainment classifyPoint(const Vector3Dd& p, double distanceTolerance) const override;
    GeometryContainment classifyPointCertified(const Vector3Dd& p, double distanceTolerance) const override;
    GeometryContainment classifyBoxCertified(const Vector3Dd& center, const Vector3Dd& halfWidths) const override;
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
    void getBounds(GeometryBounds& result) const override;
    virtual double* getMinMax();

    double getRadius() const;
    // Borrowed reference to this object's intrinsic radius; valid while it lives.
    const double& getRadiusReference() const { return radius; }
    // Signed squared implicit function: negative inside, positive outside.
    double implicitValue(const Vector3Dd& point) const { return point.dotProduct(point) - radiusSquared; }
    // Fast normal for a known nonzero surface point in local coordinates.
    Vector3Dd surfaceNormalAtNonzeroPoint(const Vector3Dd& point) const { return point.normalizedFast(); }
    double getRadiusSquared() const;
    void setRadius(double value);

    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid();
    PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid(int meridians, int parallels);

    Vector3Dd spherePosition(double theta, double phi);
    Vector3Dd sphereNormal(double theta, double phi);
    Vector3Dd sphereTangent(double theta, double phi);
    Vector3Dd sphereBinormal(double theta, double phi);
};

inline int Sphere::intersectionRoots(const Vector3Dd& p, const Vector3Dd& d,
    double radius, const RayCrossingQuery& query, double& nearT, double& farT)
{
    const double lengthSquared = d.dotProduct(d);
    if (!(lengthSquared > 0.0) || query.minT > query.maxT) return 0;
    const double directionLength = std::sqrt(lengthSquared);
    const Vector3Dd unitDirection = d.multiply(1.0 / directionLength);
    const double radiusSquared = radius * radius;
    const double ocSquared = p.dotProduct(p);
    const bool inside = ocSquared < radiusSquared;
    const double closest = -p.dotProduct(unitDirection);
    if (!inside && closest < query.minimumExteriorApproach) return 0;
    const double chordSquared = radiusSquared - ocSquared + closest * closest;
    if (chordSquared < query.minimumChordSquared) return 0;
    const double halfChord = std::sqrt(chordSquared);
    farT = (closest + halfChord) / directionLength;
    nearT = (closest - halfChord) / directionLength;
    const bool nearValid = nearT >= query.minT && nearT <= query.maxT;
    const bool farValid = farT >= query.minT && farT <= query.maxT;
    if (!nearValid && !farValid) return 0;
    if (!nearValid) nearT = farT;
    if (!farValid) farT = nearT;
    return nearT == farT ? 1 : 2;
}

#endif
