#ifndef VSDK_BLOB_SOLID_H
#define VSDK_BLOB_SOLID_H

#include "vsdk/toolkit/environment/geometry/volume/BlobComponent.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobInfluenceInterval.h"
#include "vsdk/toolkit/environment/geometry/volume/ImplicitSolid.h"

// Blob of spherical field components (PovRay 1.0 blob): the solid where the
// summed component density exceeds the threshold, F = threshold - density.
// Along a ray each set of overlapping influence spheres gives a quartic in the
// ray distance, solved by the closed-form quartic solver or, when requested, by
// Sturm sequences. Certified containment encloses the exact field
// strength * (1 - d^2/radius^2)^2; bounds are the boxes of positive components
// for a nonnegative threshold, and unbounded for a negative one.
class BlobSolid : public ImplicitSolid {
public:
    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;
    static constexpr double SOLVER_EPSILON = 1.0e-10;
    // Rays grazing an influence sphere closer than this are ignored, and
    // roots with a squared gradient below it are rejected as spurious.
    static constexpr double GRAZING_LIMIT = 1.0e-5;
private:
    java::ArrayList<BlobComponent> components;
    double threshold;
    bool sturm;
    long int determineInfluences(const Vector3Dd& origin, const Vector3Dd& direction,
        double influenceStart, java::ArrayList<BlobInfluenceInterval>& intervals) const;
    FloatingInterval clampedFalloff(long int index, const FloatingInterval& x,
        const FloatingInterval& y, const FloatingInterval& z) const;
public:
    BlobSolid(double threshold, bool useSturmSequences);
    void addComponent(const BlobComponent& component);
    long int getComponentCount() const { return components.size(); }
    const BlobComponent& getComponent(long int index) const { return components[index]; }
    double getThreshold() const { return threshold; }
    bool usesSturmSequences() const { return sturm; }
    // Summed component density at the point.
    double evaluateDensity(const Vector3Dd& point) const;
    // Gradient of threshold - density, pointing out of the solid.
    Vector3Dd gradient(const Vector3Dd& point) const;
    // Appends the PovRay ray roots: distances d >= minimumDistance along the
    // unit direction, inside the influence intervals that start no earlier than
    // influenceStart (earlier starts are moved to 0), whose gradient is usable.
    void intersect(const Vector3Dd& origin, const Vector3Dd& unitDirection,
        double minimumDistance, double influenceStart, java::ArrayList<double>& distances) const;

    Geometry* copyGeometry() const override;
    double evaluateField(const Vector3Dd& point) const override { return threshold - evaluateDensity(point); }
    Vector3Dd evaluateFieldGradient(const Vector3Dd& point) const override { return gradient(point); }
    FloatingInterval encloseField(const FloatingInterval& x, const FloatingInterval& y,
        const FloatingInterval& z) const override;
    bool encloseFieldGradient(const FloatingInterval& x, const FloatingInterval& y,
        const FloatingInterval& z, FloatingInterval& gx, FloatingInterval& gy,
        FloatingInterval& gz) const override;
    bool findRayRoots(const Vector3Dd& origin, const Vector3Dd& unitDirection,
        double maximumDistance, java::ArrayList<double>& distances) const override;
    void getBounds(GeometryBounds& result) const override;
};
#endif
