#ifndef __QUADRIC__
#define __QUADRIC__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"

class Quadric : public TransformedGeometry {
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
    void setObjectVpConstant(double value) { objectVpConstant = value; }
    bool isConstantCached() const { return constantCached; }
    void setConstantCached(bool value) { constantCached = value; }
    bool hasNonZeroSquareTerm() const { return nonZeroSquareTerm; }
    void setNonZeroSquareTerm(bool value) { nonZeroSquareTerm = value; }

    static int intersectQuadric(
        RayWithSegments *ray, Quadric *shape, double *depth1, double *depth2);

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    void updateSquareTermFlag();

    Vector3Dd object2Terms;
    Vector3Dd objectMixedTerms;
    Vector3Dd objectTerms;
    double objectConstant;
    double objectVpConstant;
    bool constantCached;
    bool nonZeroSquareTerm;

    static void quadricToMatrix(const Quadric *quadric, Matrix4x4d *matrix);
    static void matrixToQuadric(const Matrix4x4d *matrix, Quadric *quadric);
    static void transformQuadric(
        Quadric *shape, const Matrix4x4d *transformationInverse);
};
#endif
