#ifndef __TRIANGLE__
#define __TRIANGLE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

class SmoothTriangle;

class Triangle : public Geometry {
  public:
    Triangle();
    Triangle(const Vector3Dd &p1, const Vector3Dd &p2, const Vector3Dd &p3,
        bool inverted = false);

    Vector3Dd &getNormalVector() { return normalVector; }
    const Vector3Dd &getNormalVector() const { return normalVector; }
    double getDistance() const { return distance; }
    void setDistance(double d) { distance = d; }
    double getVpNormDotOrigin() const { return vpNormDotOrigin; }
    void setVpNormDotOrigin(double value) { vpNormDotOrigin = value; }
    bool isVpCached() const { return vpCached; }
    void setVpCached(bool value) { vpCached = value; }
    unsigned int getDominantAxis() const { return dominantAxis; }
    void setDominantAxis(unsigned int value) { dominantAxis = value; }
    bool isInverted() const { return inverted; }
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
    void setDegenerateFlag(bool value) { degenerateFlag = value; }

    static int computeTriangle(Triangle *triangle);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  protected:
    Triangle(const Vector3Dd &normalVector, double distance,
        double vpNormDotOrigin, bool vpCached, unsigned int dominantAxis,
        bool inverted, unsigned int vAxis, const Vector3Dd &p1,
        const Vector3Dd &p2, const Vector3Dd &p3, bool degenerateFlag);
    virtual void swapVertexNormals() {}
    virtual void finalizeComputation() {}
    static void computeSmoothTriangle(SmoothTriangle *triangle);

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

#include "environment/geometry/element/SmoothTriangle.h"

#endif
