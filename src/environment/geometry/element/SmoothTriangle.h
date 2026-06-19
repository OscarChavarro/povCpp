#ifndef __SMOOTH_TRIANGLE__
#define __SMOOTH_TRIANGLE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/Triangle.h"
#include "environment/geometry/element/Triangle.h"

class SmoothTriangle : public Triangle {
  public:
    SmoothTriangle();
    SmoothTriangle(const Vector3Dd &p1, const Vector3Dd &n1,
        const Vector3Dd &p2, const Vector3Dd &n2,
        const Vector3Dd &p3, const Vector3Dd &n3,
        bool inverted = false);

    Vector3Dd &getN1() { return n1; }
    const Vector3Dd &getN1() const { return n1; }
    Vector3Dd &getN2() { return n2; }
    const Vector3Dd &getN2() const { return n2; }
    Vector3Dd &getN3() { return n3; }
    const Vector3Dd &getN3() const { return n3; }
    Vector3Dd &getPerp() { return perp; }
    const Vector3Dd &getPerp() const { return perp; }
    double getBaseDelta() const { return baseDelta; }
    void setBaseDelta(double value) { baseDelta = value; }

    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  protected:
    SmoothTriangle(const Vector3Dd &normalVector, double distance,
        double vpNormDotOrigin, bool vpCached, unsigned int dominantAxis,
        bool inverted, unsigned int vAxis, const Vector3Dd &p1,
        const Vector3Dd &p2, const Vector3Dd &p3, bool degenerateFlag,
        const Vector3Dd &n1, const Vector3Dd &n2, const Vector3Dd &n3,
        const Vector3Dd &perp, double baseDelta);
    void swapVertexNormals() override;
    void finalizeComputation() override;

  private:
    Vector3Dd n1;
    Vector3Dd n2;
    Vector3Dd n3;
    Vector3Dd perp;
    double baseDelta;
};

#endif
