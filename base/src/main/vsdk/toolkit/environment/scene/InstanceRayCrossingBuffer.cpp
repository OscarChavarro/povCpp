#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/InstanceRayCrossingBuffer.h"

InstanceRayCrossingBuffer::InstanceRayCrossingBuffer(const RayInstanceStep& value,
    const SimpleBody* owner, Material* appearance) : instance(value), body(owner),
    material(appearance), failed(false) {}
void InstanceRayCrossingBuffer::crossing(const RayCrossing& event)
{
    if (failed) return;
    RayBoundary boundary(event);
    if (!boundary.appendInstance(instance) || !boundaries.add(boundary) || !kinds.add(event.kind) ||
        !bodies.add(event.body ? event.body : body) || !materials.add(event.material ? event.material : material))
        failed = true;
}
bool InstanceRayCrossingBuffer::isValid() const { return !failed; }
void InstanceRayCrossingBuffer::deliver(RayCrossingSink& sink) const
{
    if (failed) return;
    for (long int i=0; i<boundaries.size(); ++i) {
        RayCrossing event(boundaries[i].t, boundaries[i].geometry, kinds[i]);
        static_cast<RayBoundary&>(event) = boundaries[i];
        event.body = bodies[i]; event.material = materials[i];
        sink.crossing(event);
    }
}
