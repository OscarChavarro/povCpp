#ifndef VSDK_INSTANCE_RAY_CROSSING_BUFFER_H
#define VSDK_INSTANCE_RAY_CROSSING_BUFFER_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"
#include "vsdk/toolkit/environment/geometry/element/RayInstanceStep.h"
class SimpleBody;
class Material;

// Buffers events so a failed source query never emits a partial result.
class InstanceRayCrossingBuffer : public RayCrossingSink {
private:
    java::ArrayList<RayBoundary> boundaries;
    java::ArrayList<RayCrossingKind> kinds;
    java::ArrayList<const SimpleBody*> bodies;
    java::ArrayList<Material*> materials;
    RayInstanceStep instance;
    const SimpleBody* body;
    Material* material;
    bool failed;
public:
    InstanceRayCrossingBuffer(const RayInstanceStep& instance, const SimpleBody* body, Material* material);
    void crossing(const RayCrossing& event) override;
    bool isValid() const;
    void deliver(RayCrossingSink& sink) const;
};
#endif
