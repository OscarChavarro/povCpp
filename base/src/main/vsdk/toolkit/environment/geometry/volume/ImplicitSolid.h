#ifndef VSDK_IMPLICIT_SOLID_H
#define VSDK_IMPLICIT_SOLID_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"

// Solid F(p) < 0 of a continuous scalar field F whose ray restriction a
// subclass can solve. This class turns the roots of F along a ray into the
// typed queries: first hit, crossings, ray classification, certified point and
// box containment, and surface detail with the outward normal grad F / |grad F|.
// Solver roots are refined by Newton steps on F before use.
// Ray membership between consecutive roots is decided by the sign of F at a
// point inside the segment, so tangent double roots do not split segments and a zero or
// non-finite sample is reported as Indeterminate. Queries need a finite minT.
class ImplicitSolid : public Solid {
public:
    // Largest relative Newton correction applied to a solver root.
    static constexpr double ROOT_POLISH_REACH = 1.0e-3;
    // Roots closer than this relative distance are one root.
    static constexpr double ROOT_MERGE_TOLERANCE = 1.0e-12;
private:
    double polishRoot(const Vector3Dd& start, const Vector3Dd& unit, double distance) const;
    RayFirstHitStatus collectRoots(const Ray& ray, const RayCrossingQuery& query,
        java::ArrayList<double>& distances, Vector3Dd& start, Vector3Dd& unit,
        double& length, double& maximumDistance) const;
    int sampleSign(const Vector3Dd& start, const Vector3Dd& unit, double distance) const;
    int enclosureSign(const Vector3Dd& point) const;
    int boxSign(const FloatingInterval& x, const FloatingInterval& y, const FloatingInterval& z,
        const Vector3Dd& center) const;
    RaySolidQueryStatus classifySegments(const java::ArrayList<double>& distances,
        const Vector3Dd& start, const Vector3Dd& unit, double maximumDistance,
        java::ArrayList<bool>& inside) const;
public:
    ~ImplicitSolid() override {}
    // Field value; the solid is F < 0.
    virtual double evaluateField(const Vector3Dd& point) const = 0;
    // Gradient of the field, pointing out of the solid.
    virtual Vector3Dd evaluateFieldGradient(const Vector3Dd& point) const = 0;
    // Certified enclosure of F over the box x by y by z; invalid when unknown.
    virtual FloatingInterval encloseField(const FloatingInterval& x,
        const FloatingInterval& y, const FloatingInterval& z) const = 0;
    // Certified enclosures of the gradient over the box, used for the
    // mean-value form F(c) + grad F(box) . (box - c); false when unavailable.
    virtual bool encloseFieldGradient(const FloatingInterval& x, const FloatingInterval& y,
        const FloatingInterval& z, FloatingInterval& gx, FloatingInterval& gy,
        FloatingInterval& gz) const;
    // Appends the distances d >= 0 at which F(origin + d*unitDirection) = 0,
    // in any order, including at least every root up to maximumDistance.
    // Returns false when the roots cannot be found.
    virtual bool findRayRoots(const Vector3Dd& origin, const Vector3Dd& unitDirection,
        double maximumDistance, java::ArrayList<double>& distances) const = 0;

    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    // Sign enclosures over the ball's bounding box prove Inside/Outside;
    // Boundary requires two certified points of opposite sign within the ball.
    GeometryContainment classifyPointCertified(const Vector3Dd& point, double tolerance) const override;
    GeometryContainment classifyBoxCertified(const Vector3Dd& center, const Vector3Dd& halfWidths) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) override;
    int doContainmentTest(const Vector3Dd& point, double tolerance) override;
    double* getMinMax() override;
};
#endif
