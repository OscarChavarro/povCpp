#ifndef __BOX__
#define __BOX__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

class Box : public Geometry {
  public:
    Box();
    Box(const Vector3Dd &minBounds, const Vector3Dd &maxBounds,
        bool inverted = false);
    Box(Matrix4x4d *transformation, Matrix4x4d *transformationInverse,
        const Vector3Dd &minBounds, const Vector3Dd &maxBounds, bool inverted);
    Box(const Box &other);
    ~Box() override;

    Matrix4x4d* getTransformation() const { return transformation; }
    void setTransformation(Matrix4x4d *value) { transformation = value; }
    Matrix4x4d* getTransformationInverse() const { return transformationInverse; }
    void setTransformationInverse(Matrix4x4d *value)
    {
        transformationInverse = value;
    }
    Vector3Dd* getBounds() { return bounds; }
    const Vector3Dd* getBounds() const { return bounds; }
    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }

    static int intersectBoxx(const RayWithSegments *ray, const Box *box,
        double *depth1, double *depth2);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        Material *material) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    Vector3Dd bounds[2];
    bool inverted;

    static int closeTo(double x, double y);
};

#endif
