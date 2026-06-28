#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/scene/Composite.h"
#include "io/pov/light/LightGeometryAdapter.h"
#include "io/pov/scene/ScenePostProcessor.h"

void
ScenePostProcessor::linkLights(SimpleBody *object, java::ArrayList<Light*> &lights)
{
    java::ArrayList<const Matrix4x4d*> transforms(8);
    ScenePostProcessor::linkLights(object, lights, transforms);
}

void
ScenePostProcessor::applyTransformsToLight(
    Light *light, const java::ArrayList<const Matrix4x4d*> &transforms)
{
    if (light == nullptr) {
        return;
    }

    Vector3Dd center = light->getCenter();
    Vector3Dd pointsAt = light->getPointsAt();
    for (long int i = transforms.size() - 1; i >= 0; i--) {
        const Matrix4x4d *transform = transforms[i];
        if (transform == nullptr) {
            continue;
        }
        center = transform->transpose().multiply(center);
        pointsAt = transform->transpose().multiply(pointsAt);
    }
    light->getCenter() = center;
    light->getPointsAt() = pointsAt;
}

void
ScenePostProcessor::linkLights(
    SimpleBody *object,
    java::ArrayList<Light*> &lights,
    java::ArrayList<const Matrix4x4d*> &transforms)
{
    const long int baseSize = transforms.size();
    if (object->getTransformation() != nullptr) {
        transforms.add(object->getTransformation());
    }
    if (object->getGeometryTransformation() != nullptr) {
        transforms.add(object->getGeometryTransformation());
    }

    if (Composite *composite = dynamic_cast<Composite *>(object)) {
        java::ArrayList<SimpleBody*> &simpleBodies = composite->getSimpleBodies();
        for (long int i = 0; i < simpleBodies.size(); i++) {
            ScenePostProcessor::linkLights(simpleBodies[i], lights, transforms);
        }
    } else {
        ScenePostProcessor::linkLightsInShape(object->getGeometry(), lights, transforms);
    }

    while (transforms.size() > baseSize) {
        transforms.remove(transforms.size() - 1);
    }
}

void
ScenePostProcessor::linkLightsInOperand(
    CsgOperand *operand,
    java::ArrayList<Light*> &lights,
    java::ArrayList<const Matrix4x4d*> &transforms)
{
    const long int baseSize = transforms.size();
    if (operand->getTransformation() != nullptr) {
        transforms.add(operand->getTransformation());
    }
    ScenePostProcessor::linkLightsInShape(operand->getGeometry(), lights, transforms);
    while (transforms.size() > baseSize) {
        transforms.remove(transforms.size() - 1);
    }
}

void
ScenePostProcessor::linkLightsInShape(
    Geometry *shape,
    java::ArrayList<Light*> &lights,
    java::ArrayList<const Matrix4x4d*> &transforms)
{
    if (ConstructiveSolidGeometry *csg = dynamic_cast<ConstructiveSolidGeometry *>(shape)) {
        java::ArrayList<CsgOperand*> &operands = csg->getOperands();
        for (long int i = 0; i < operands.size(); i++) {
            ScenePostProcessor::linkLightsInOperand(operands[i], lights, transforms);
        }
    } else if (LightGeometryAdapter *lightAdapter =
                   dynamic_cast<LightGeometryAdapter *>(shape)) {
        ScenePostProcessor::applyTransformsToLight(lightAdapter->getLight(), transforms);
        lights.add(lightAdapter->getLight());
    }
}
