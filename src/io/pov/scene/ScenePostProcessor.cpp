#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/light/LightGeometryAdapter.h"
#include "io/pov/scene/ScenePostProcessor.h"

void
ScenePostProcessor::linkLights(BoundedGeometry *object, Light *&lightHead)
{
    if (Composite *composite = dynamic_cast<Composite *>(object)) {
        java::ArrayList<BoundedGeometry*> &simpleBodies = composite->getSimpleBodies();
        for (long int i = simpleBodies.size() - 1; i >= 0; i--) {
            ScenePostProcessor::linkLights(simpleBodies[i], lightHead);
        }
    } else {
        ScenePostProcessor::linkLightsInShape(
            static_cast<SimpleBody*>(object->getGeometry()), lightHead);
    }
}

void
ScenePostProcessor::linkLightsInShape(SimpleBody *shape, Light *&lightHead)
{
    SimpleBody *tempShape;

    if (CSG *csg = dynamic_cast<CSG *>(shape->getGeometry())) {
        java::ArrayList<TransformableElement*> &shapes = csg->getShapes();
        for (long int i = shapes.size() - 1; i >= 0; i--) {
            tempShape = static_cast<SimpleBody*>(shapes[i]);
            ScenePostProcessor::linkLightsInShape(tempShape, lightHead);
        }
    } else if (LightGeometryAdapter *lightAdapter =
                   dynamic_cast<LightGeometryAdapter *>(shape->getGeometry())) {
        Light *light = lightAdapter->getLight();
        light->setNextLightSource(lightHead);
        lightHead = light;
    }
}
