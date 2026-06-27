#ifndef __TRIANGLE__
#define __TRIANGLE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"

class Triangle : public TransformedGeometry {
  public:
    Triangle();
    Triangle(const Vector3Dd &p1, const Vector3Dd &p2, const Vector3Dd &p3,
        bool inverted = false);
    Triangle(const Triangle &other) :
        normalVector(other.normalVector),
        distance(other.distance),
        vpNormDotOrigin(other.vpNormDotOrigin),
        vpCached(other.vpCached),
        dominantAxis(other.dominantAxis),
        inverted(other.inverted),
        vAxis(other.vAxis),
        p1(other.p1),
        p2(other.p2),
        p3(other.p3),
        degenerateFlag(other.degenerateFlag)
    {}

    Vector3Dd &getNormalVector() { return normalVector; }
    double getDistance() const { return distance; }
    void setDistance(double d) { distance = d; }
    void setInverted(bool value) { inverted = value; }
    void toggleInverted() { inverted ^= true; }
    unsigned int getVAxis() const { return vAxis; }
    void setVAxis(unsigned int value) { vAxis = value; }
    Vector3Dd &getP1() { return p1; }
    const Vector3Dd &getP1() const { return p1; }
    Vector3Dd &getP2() { return p2; }
    const Vector3Dd &getP2() const { return p2; }
    Vector3Dd &getP3() { return p3; }
    const Vector3Dd &getP3() const { return p3; }
    bool isDegenerate() const { return degenerateFlag; }

    static int computeTriangle(Triangle *triangle);

    AxisAlignedBox getMinMax() const override;

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

  protected:
    virtual void swapVertexNormals() {}
    virtual void finalizeComputation() {}

  private:
    Vector3Dd normalVector;
    double distance;
    double vpNormDotOrigin;
    unsigned int vpCached : 1;
    unsigned int dominantAxis : 2;
    unsigned int inverted : 1;
    unsigned int vAxis : 2;
    Vector3Dd p1;
    Vector3Dd p2;
    Vector3Dd p3;
    bool degenerateFlag;

    static int max3Axis(double x, double y, double z);
    static void findTriangleDominantAxis(Triangle *triangle);
    static int intersectTriangle(
        RayWithSegments *ray, Triangle *triangle, double *depth);
};

#endif
