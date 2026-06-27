#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/scene/Composite.h"
#include "io/pov/light/LightGeometryAdapter.h"
#include "io/pov/scene/ScenePostProcessor.h"

void
ScenePostProcessor::linkLights(SimpleBody *object, java::ArrayList<Light*> &lights)
{
    if (Composite *composite = dynamic_cast<Composite *>(object)) {
        java::ArrayList<SimpleBody*> &simpleBodies = composite->getSimpleBodies();
        for (long int i = 0; i < simpleBodies.size(); i++) {
            ScenePostProcessor::linkLights(simpleBodies[i], lights);
        }
    } else {
        ScenePostProcessor::linkLightsInShape(object->getGeometry(), lights);
    }
}

void
ScenePostProcessor::linkLightsInShape(Geometry *shape, java::ArrayList<Light*> &lights)
{
    if (ConstructiveSolidGeometry *csg = dynamic_cast<ConstructiveSolidGeometry *>(shape)) {
        java::ArrayList<CsgOperand*> &operands = csg->getOperands();
        for (long int i = 0; i < operands.size(); i++) {
            ScenePostProcessor::linkLightsInShape(operands[i]->getGeometry(), lights);
        }
    } else if (LightGeometryAdapter *lightAdapter =
                   dynamic_cast<LightGeometryAdapter *>(shape)) {
        lights.add(lightAdapter->getLight());
    }
}
