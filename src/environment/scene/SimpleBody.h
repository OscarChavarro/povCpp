#ifndef __SIMPLE_BODY__
#define __SIMPLE_BODY__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/material/Material.h"

class SimpleBody {
  private:
    TransformedGeometry *const geometry = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;

  public:
    SimpleBody() {}
    SimpleBody(TransformedGeometry *geometry, Material *material, ColorRgba *shapeColor);
    SimpleBody(const SimpleBody &other);
    ~SimpleBody();

    TransformedGeometry* getGeometry() const { return geometry; }
    Geometry* getWrappedGeometry() const { return geometry; }
    Material* getMaterial() const { return material; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    ColorRgba* ensureShapeColor();
    void prependMaterialLayers(Material *newHead);

    bool doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out);
    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue);
    int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *material);
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance);
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint);
    void normal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const PovRayRendererConfiguration *config);
    void *copy();
    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();
};

#endif
