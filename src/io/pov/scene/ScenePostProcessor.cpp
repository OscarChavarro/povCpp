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
ScenePostProcessor::linkLightsInShape(TransformedGeometry *shape, java::ArrayList<Light*> &lights)
{
    if (ConstructiveSolidGeometry *csg = dynamic_cast<ConstructiveSolidGeometry *>(shape)) {
        java::ArrayList<TransformedGeometry*> &shapes = csg->getShapes();
        for (long int i = 0; i < shapes.size(); i++) {
            ScenePostProcessor::linkLightsInShape(shapes[i], lights);
        }
    } else if (LightGeometryAdapter *lightAdapter =
                   dynamic_cast<LightGeometryAdapter *>(shape)) {
        lights.add(lightAdapter->getLight());
    }
}
