#ifndef __LIGHT_GEOMETRY_ADAPTER__
#define __LIGHT_GEOMETRY_ADAPTER__

#include "environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/light/Light.h"

class LightGeometryAdapter : public Geometry {
  public:
    explicit LightGeometryAdapter(Light *lightSource) : light(lightSource) {}
    LightGeometryAdapter(const LightGeometryAdapter &other);
    ~LightGeometryAdapter() override { delete light; }

    Light *getLight() const { return light; }
    void setLight(Light *lightSource) { light = lightSource; }
    // Hands ownership to the caller (used once, by ScenePostProcessor::
    // linkLights, to move each linked light into Scene::lightSources - see
    // Scene.cpp). Adapters that never get linked (declared-but-unused
    // #declare copies, still cleaned up via the constants table) keep a
    // non-null light and free it normally through the destructor below.
    Light *releaseLight() { Light * const result = light; light = nullptr; return result; }

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;

  private:
    static Light *cloneLight(const Light *light);

    Light *light;
};

#endif
