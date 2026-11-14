
#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/light/SpotLight.h"
#include "environment/scene/RayCastingComposite.h"
#include "io/pov/scene/ScenePostProcessor.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

void
ScenePostProcessor::linkLights(PovrayBody *object, java::ArrayList<Light*> &lights)
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

    SpotLight *spotLight = dynamic_cast<SpotLight *>(light);

    Vector3Dd position = light->getPosition();
    Vector3Dd pointsAt = spotLight != nullptr ? spotLight->getPointsAt() : Vector3Dd();
    for (long int i = transforms.size() - 1; i >= 0; i--) {
        const Matrix4x4d *transform = transforms[i];
        if (transform == nullptr) {
            continue;
        }
        position = transform->transpose().multiply(position);
        if (spotLight != nullptr) {
            pointsAt = transform->transpose().multiply(pointsAt);
        }
    }
    light->setPosition(position);
    if (spotLight != nullptr) {
        spotLight->setPointsAt(pointsAt);
    }
}

void
ScenePostProcessor::linkLights(
    PovrayBody *object,
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

    if (RayCastingComposite *composite = dynamic_cast<RayCastingComposite *>(object)) {
        java::ArrayList<PovrayBody*> &simpleBodies = composite->getSimpleBodies();
        for (long int i = 0; i < simpleBodies.size(); i++) {
            ScenePostProcessor::linkLights(simpleBodies[i], lights, transforms);
        }
    } else {
        ScenePostProcessor::linkLightsInShape(object, lights, transforms);
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
    ScenePostProcessor::linkLightsInShape(operand, lights, transforms);
    while (transforms.size() > baseSize) {
        transforms.remove(transforms.size() - 1);
    }
}

void
ScenePostProcessor::linkLightsInShape(
    PovrayBody *body,
    java::ArrayList<Light*> &lights,
    java::ArrayList<const Matrix4x4d*> &transforms)
{
    if (ConstructiveSolidGeometry *csg = body->getCsg()) {
        const java::ArrayList<CsgOperand*> &operands = csg->getOperands();
        for (long int i = 0; i < operands.size(); i++) {
            ScenePostProcessor::linkLightsInOperand(operands[i], lights, transforms);
        }
    } else if (body->getLight() != nullptr) {
        // The scene receives a placed copy; the body keeps its light.
        Light *linked = body->getLight()->copy();
        ScenePostProcessor::applyTransformsToLight(linked, transforms);
        lights.add(linked);
    }
}
