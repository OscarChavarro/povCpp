#ifndef __QUADRIC__
#define __QUADRIC__

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
        RayWithTracingState *ray, Quadric *shape, double *depth1, double *depth2);

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    bool doIntersectionFirstHit(
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    BoundingVolumeHierarchy *createBoundingVolume() const override;
    void *copy() override;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;

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
