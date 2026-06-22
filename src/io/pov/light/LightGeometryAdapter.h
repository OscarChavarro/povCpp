#ifndef __LIGHT_GEOMETRY_ADAPTER__
#define __LIGHT_GEOMETRY_ADAPTER__

#include "environment/geometry/Geometry.h"
#include "environment/light/Light.h"

class LightGeometryAdapter : public Geometry {
  public:
    explicit LightGeometryAdapter(Light *lightSource) : light(lightSource) {}
    LightGeometryAdapter(const LightGeometryAdapter &other) :
        light(other.light != nullptr ? other.light->copy() : nullptr)
    {
    }
    ~LightGeometryAdapter() override { delete light; }

    Light *getLight() const { return light; }
    void setLight(Light *lightSource) { light = lightSource; }

    int allIntersections(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *material) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    Light *light;
};

#endif
