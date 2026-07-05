#ifndef __QUADRIC__
#define __QUADRIC__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

class Quadric : public Geometry {
  public:
    Quadric();
    Quadric(const Vector3Dd &object2Terms, const Vector3Dd &objectMixedTerms,
        const Vector3Dd &objectTerms, double objectConstant);
    Quadric(const Vector3Dd &object2Terms, const Vector3Dd &objectMixedTerms,
        const Vector3Dd &objectTerms, double objectConstant,
        double objectVpConstant, bool constantCached, bool nonZeroSquareTerm);
    Quadric(const Quadric &other) :
        object2Terms(other.object2Terms),
        objectMixedTerms(other.objectMixedTerms),
        objectTerms(other.objectTerms),
        objectConstant(other.objectConstant),
        objectVpConstant(other.objectVpConstant),
        constantCached(other.constantCached),
        nonZeroSquareTerm(other.nonZeroSquareTerm)
    {}

    Vector3Dd &getObject2Terms() { return object2Terms; }
    const Vector3Dd &getObject2Terms() const { return object2Terms; }
    Vector3Dd &getObjectMixedTerms() { return objectMixedTerms; }
    const Vector3Dd &getObjectMixedTerms() const { return objectMixedTerms; }
    Vector3Dd &getObjectTerms() { return objectTerms; }
    const Vector3Dd &getObjectTerms() const { return objectTerms; }
    double getObjectConstant() const { return objectConstant; }
    void setObjectConstant(double value) { objectConstant = value; }
    double getObjectVpConstant() const { return objectVpConstant; }
    void setObjectVpConstant(double value) const { objectVpConstant = value; }
    bool isConstantCached() const { return constantCached; }
    void setConstantCached(bool value) const { constantCached = value; }
    bool hasNonZeroSquareTerm() const { return nonZeroSquareTerm; }
    void setNonZeroSquareTerm(bool value) { nonZeroSquareTerm = value; }

    static int intersectQuadric(
        RayWithSegments *ray, Quadric *shape, double *depth1, double *depth2);

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    bool doIntersectionFirstHit(
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    AxisAlignedBoundingBox getMinMax() const override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void invertGeometry() override;

  private:
    void updateSquareTermFlag();

    Vector3Dd object2Terms;
    Vector3Dd objectMixedTerms;
    Vector3Dd objectTerms;
    double objectConstant;
    mutable double objectVpConstant;
    mutable bool constantCached;
    bool nonZeroSquareTerm;
};
#endif
