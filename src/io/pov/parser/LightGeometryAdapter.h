#ifndef __LIGHT_GEOMETRY_ADAPTER_H__
#define __LIGHT_GEOMETRY_ADAPTER_H__

#include "environment/geometry/Geometry.h"
#include "environment/light/Light.h"

class LightGeometryAdapter : public Geometry {
  public:
    explicit LightGeometryAdapter(Light *lightSource) : light(lightSource) {}

    Light *light;

    int allIntersections(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForOwner(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        SimpleBody *owner) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;
};

#endif
