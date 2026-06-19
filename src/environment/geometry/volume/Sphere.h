#ifndef __SPHERE__
#define __SPHERE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

class Sphere : public Geometry {
  public:
    Sphere();
    Sphere(const Vector3Dd &center, double radius, bool inverted = false);
    Sphere(const Vector3Dd &center, double radius, double radiusSquared,
        double inverseRadius, const Vector3Dd &vpOtoC, double vpOCSquared,
        short vpInside, bool vpCached, bool inverted);
    Sphere(const Sphere &other) = default;

    Vector3Dd& getCenter() { return center; }
    const Vector3Dd& getCenter() const { return center; }
    double getRadius() const { return radius; }
    void setRadius(double r) { radius = r; }
    double getRadiusSquared() const { return radiusSquared; }
    void setRadiusSquared(double value) { radiusSquared = value; }
    double getInverseRadius() const { return inverseRadius; }
    void setInverseRadius(double ir) { inverseRadius = ir; }
    Vector3Dd &getVpOtoC() { return vpOtoC; }
    const Vector3Dd &getVpOtoC() const { return vpOtoC; }
    double getVpOCSquared() const { return vpOCSquared; }
    void setVpOCSquared(double value) { vpOCSquared = value; }
    short getVpInside() const { return vpInside; }
    void setVpInside(short value) { vpInside = value; }
    bool isVpCached() const { return vpCached; }
    void setVpCached(bool value) { vpCached = value; }
    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }
    void toggleInverted() { inverted ^= true; }

    static int intersectSphere(const RayWithSegments *ray, Sphere *sphere,
        double *depth1, double *depth2);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForOwner(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        SimpleBody *owner) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    void updateRadiusState(double radius);

    Vector3Dd center;
    double radius;
    double radiusSquared;
    double inverseRadius;
    Vector3Dd vpOtoC;
    double vpOCSquared;
    short vpInside;
    bool vpCached;
    bool inverted;
};

#endif
