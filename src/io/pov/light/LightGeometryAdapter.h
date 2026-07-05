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

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

  private:
    Light *light;
};

#endif
