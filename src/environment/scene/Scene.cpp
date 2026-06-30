#include "java/util/ArrayList.txx"

#include "vsdk/toolkit/environment/camera/Camera.h"

#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/scene/Composite.h"
#include "environment/scene/Scene.h"

namespace {
CameraSnapshot
defaultViewPoint()
{
    const Vector3Dd location(0.0, 0.0, 0.0);
    const Vector3Dd direction(0.0, 0.0, 1.0);
    const Vector3Dd up(0.0, 1.0, 0.0);
    const Vector3Dd right(1.33, 0.0, 0.0);
    const Vector3Dd front = direction.normalizedFast();
    const Vector3Dd left = right.normalizedFast().multiply(-1.0);
    const Vector3Dd upNormalized = up.normalizedFast();

    return CameraSnapshot(
        location,
        front,
        left,
        upNormalized,
        Camera::PROJECTION_MODE_PERSPECTIVE,
        1.0,
        0.0,
        0.0,
        direction,
        up,
        right);
}

bool
isBakeableSimpleBody(SimpleBody *object)
{
    if (object == nullptr) {
        return false;
    }
    return dynamic_cast<Composite *>(object) == nullptr;
}

Matrix4x4d
copyOrIdentity(const Matrix4x4d *matrix)
{
    return matrix != nullptr ? Matrix4x4d(*matrix) : Matrix4x4d::identityMatrix();
}

bool
hasFiniteInteriorBounds(const Scene::BakedCsgOperand &operand)
{
    if (!operand.bounded || !operand.cullSafe || operand.bakedCsgIndex >= 0) {
        return false;
    }

    const Geometry *geometry = operand.geometry;
    if (const Sphere *sphere = dynamic_cast<const Sphere *>(geometry)) {
        return !sphere->isInverted();
    }
    if (const Box *box = dynamic_cast<const Box *>(geometry)) {
        return !box->isInverted();
    }
    return false;
}

bool
areSeparated(const AxisAlignedBox &left, const AxisAlignedBox &right)
{
    return
        left.max.x() < right.min.x() || right.max.x() < left.min.x() ||
        left.max.y() < right.min.y() || right.max.y() < left.min.y() ||
        left.max.z() < right.min.z() || right.max.z() < left.min.z();
}

bool
hasPairwiseDisjointFiniteOperands(
    const java::ArrayList<Scene::BakedCsgOperand> &operands)
{
    if (operands.size() == 0) {
        return false;
    }
    for (long int i = 0; i < operands.size(); i++) {
        if (!hasFiniteInteriorBounds(operands[i])) {
            return false;
        }
    }
    for (long int i = 0; i < operands.size(); i++) {
        for (long int j = i + 1; j < operands.size(); j++) {
            if (!areSeparated(operands[i].bakedBounds, operands[j].bakedBounds)) {
                return false;
            }
        }
    }
    return true;
}

void
classifyBakedConstructiveSolidGeometry(
    Scene::BakedConstructiveSolidGeometry &baked)
{
    if (baked.geometryType != BooleanSetOperations::DIFFERENCE &&
        baked.geometryType != BooleanSetOperations::INTERSECTION) {
        bool allPlanes = baked.operands.size() > 0;
        for (long int i = 0; i < baked.operands.size(); i++) {
            if (!baked.operands[i].isInfinitePlane) {
                allPlanes = false;
            }
        }
        if (baked.topLevel && allPlanes) {
            baked.specialization = Scene::BakedCsgSpecialization::TopLevelPlaneUnion;
            return;
        }
        if (hasPairwiseDisjointFiniteOperands(baked.operands)) {
            baked.specialization = Scene::BakedCsgSpecialization::DisjointBoundedUnion;
        }
        return;
    }

    if (baked.geometryType != BooleanSetOperations::INTERSECTION ||
        baked.operands.size() < 2) {
        return;
    }

    int coreIndex = -1;
    for (long int i = 0; i < baked.operands.size(); i++) {
        const Scene::BakedCsgOperand &operand = baked.operands[i];
        if (operand.isInfinitePlane && operand.bakedCsgIndex < 0) {
            continue;
        }
        if (coreIndex >= 0) {
            return;
        }
        coreIndex = i;
    }
    if (coreIndex >= 0) {
        baked.specialization =
            Scene::BakedCsgSpecialization::SingleCorePlaneIntersection;
        baked.specializationCoreOperandIndex = coreIndex;
    }
}

Scene::CompiledTracingObject
compileTracingObject(SimpleBody *object, Scene::CompiledTracingScene &compiledScene);

int
compileConstructiveSolidGeometry(
    ConstructiveSolidGeometry *geometry,
    Scene::CompiledTracingScene &compiledScene);

void
compileTracingObjects(
    const java::ArrayList<SimpleBody*> &objects,
    Scene::CompiledTracingScene &compiledScene,
    java::ArrayList<Scene::CompiledTracingObject> &out)
{
    out.clear();
    out.reserve(objects.size());
    for (long int i = 0; i < objects.size(); i++) {
        if (objects[i] == nullptr) {
            continue;
        }
        out.add(compileTracingObject(objects[i], compiledScene));
    }
}

Scene::BakedSimpleBody
bakeSimpleBody(SimpleBody *object)
{
    Scene::BakedSimpleBody baked;
    baked.object = object;
    baked.geometry = object->getGeometry();
    baked.geometryMaterial = object->getGeometryMaterial();
    baked.objectTexture = object->getObjectTexture();
    baked.objectColor = object->getObjectColor();
    baked.worldBounds = object->getAABB();
    baked.bounded = !baked.worldBounds.isUnbounded();
    baked.noShadowFlag = object->getNoShadowFlag();
    baked.hasObjectTransform = object->getTransformation() != nullptr;
    baked.hasGeometryTransform = object->getGeometryTransformation() != nullptr;
    baked.hasBoundingShapes = object->getBoundingShapes().size() > 0;
    baked.hasClippingShapes = object->getClippingShapes().size() > 0;
    baked.objectToWorld = copyOrIdentity(object->getTransformation());
    baked.worldToObject = copyOrIdentity(object->getTransformationInverse());

    baked.geometryToObject =
        copyOrIdentity(object->getGeometryTransformation());
    baked.objectToGeometry =
        copyOrIdentity(object->getGeometryTransformationInverse());
    baked.geometryToWorld = baked.objectToWorld.multiply(baked.geometryToObject);
    baked.worldToGeometry = baked.objectToGeometry.multiply(baked.worldToObject);
    return baked;
}

Scene::BakedCsgOperand
bakeCsgOperand(
    CsgOperand *operand,
    Scene::CompiledTracingScene &compiledScene)
{
    Scene::BakedCsgOperand baked;
    baked.operand = operand;
    baked.geometry = operand->getGeometry();
    baked.material = operand->getMaterial();
    baked.hasTransform = operand->getTransformation() != nullptr;
    baked.objectToLocal = copyOrIdentity(operand->getTransformation());
    baked.localToObject = copyOrIdentity(operand->getTransformationInverse());
    baked.bakedBounds = operand->getBakedBounds();
    baked.bounded = operand->hasBoundedBakedBounds();
    baked.isInfinitePlane = dynamic_cast<InfinitePlane *>(baked.geometry) != nullptr;
    if (InfinitePlane *plane = dynamic_cast<InfinitePlane *>(baked.geometry)) {
        baked.planeNormal = plane->getNormalVector();
        baked.planeDistance = plane->getDistance();
    }
    baked.cullSafe =
        baked.bounded &&
        (dynamic_cast<Sphere *>(baked.geometry) != nullptr ||
         dynamic_cast<Box *>(baked.geometry) != nullptr ||
         dynamic_cast<Blob *>(baked.geometry) != nullptr ||
         dynamic_cast<Quadric *>(baked.geometry) != nullptr ||
         dynamic_cast<HeightField *>(baked.geometry) != nullptr);

    if (ConstructiveSolidGeometry *nestedCsg =
            dynamic_cast<ConstructiveSolidGeometry *>(baked.geometry)) {
        baked.bakedCsgIndex =
            compileConstructiveSolidGeometry(nestedCsg, compiledScene);
        baked.cullSafe = false;
        baked.isInfinitePlane = false;
    }
    return baked;
}

Scene::BakedConstructiveSolidGeometry
bakeConstructiveSolidGeometry(
    ConstructiveSolidGeometry *geometry,
    Scene::CompiledTracingScene &compiledScene)
{
    Scene::BakedConstructiveSolidGeometry baked;
    baked.geometry = geometry;
    baked.geometryType = geometry->getGeometryType();
    if (ConstructiveSolidGeometryByRaySegment *raySegment =
            dynamic_cast<ConstructiveSolidGeometryByRaySegment *>(geometry)) {
        baked.algorithm = Scene::BakedCsgAlgorithm::RaySegments;
        baked.topLevel = raySegment->isTopLevel();
    } else {
        baked.algorithm = Scene::BakedCsgAlgorithm::MorganRules;
        baked.topLevel = false;
    }

    const java::ArrayList<CsgOperand*> &operands = geometry->getOperands();
    baked.operands.reserve(operands.size());
    for (long int i = 0; i < operands.size(); i++) {
        if (operands[i] == nullptr) {
            continue;
        }
        baked.operands.add(bakeCsgOperand(operands[i], compiledScene));
    }
    classifyBakedConstructiveSolidGeometry(baked);
    return baked;
}

int
compileConstructiveSolidGeometry(
    ConstructiveSolidGeometry *geometry,
    Scene::CompiledTracingScene &compiledScene)
{
    const int index = compiledScene.bakedCsgs.size();
    compiledScene.bakedCsgs.add(Scene::BakedConstructiveSolidGeometry());
    compiledScene.bakedCsgs.set(
        index,
        bakeConstructiveSolidGeometry(geometry, compiledScene));
    return index;
}

Scene::BakedComposite
bakeComposite(Composite *object, Scene::CompiledTracingScene &compiledScene)
{
    Scene::BakedComposite baked;
    baked.object = object;
    baked.worldBounds = object->getAABB();
    baked.bounded = !baked.worldBounds.isUnbounded();
    baked.noShadowFlag = object->getNoShadowFlag();
    baked.hasObjectTransform = object->getTransformation() != nullptr;
    baked.hasBoundingShapes = object->getBoundingShapes().size() > 0;
    baked.hasClippingShapes = object->getClippingShapes().size() > 0;
    baked.objectToWorld = copyOrIdentity(object->getTransformation());
    baked.worldToObject = copyOrIdentity(object->getTransformationInverse());

    compileTracingObjects(
        object->getBoundingShapes(),
        compiledScene,
        baked.boundingObjects);
    compileTracingObjects(
        object->getClippingShapes(),
        compiledScene,
        baked.clippingObjects);

    const java::ArrayList<SimpleBody*> &children = object->getSimpleBodies();
    baked.childObjects.reserve(children.size());
    baked.boundedChildObjects.reserve(children.size());
    baked.unboundedChildObjects.reserve(children.size());
    for (long int i = 0; i < children.size(); i++) {
        Scene::CompiledTracingObject childEntry =
            compileTracingObject(children[i], compiledScene);
        baked.childObjects.add(childEntry);
        if (childEntry.bounded) {
            baked.boundedChildObjects.add(childEntry);
        } else {
            baked.unboundedChildObjects.add(childEntry);
        }
    }
    return baked;
}

Scene::CompiledTracingObject
compileTracingObject(SimpleBody *object, Scene::CompiledTracingScene &compiledScene)
{
    Scene::CompiledTracingObject entry;
    entry.object = object;
    entry.bounds = object->getAABB();
    entry.bounded = !entry.bounds.isUnbounded();
    entry.castsShadow = !object->getNoShadowFlag();
    if (isBakeableSimpleBody(object)) {
        entry.bakedSimpleBodyIndex = compiledScene.bakedSimpleBodies.size();
        compiledScene.bakedSimpleBodies.add(Scene::BakedSimpleBody());
        Scene::BakedSimpleBody baked = bakeSimpleBody(object);
        if (ConstructiveSolidGeometry *csg =
                dynamic_cast<ConstructiveSolidGeometry *>(baked.geometry)) {
            baked.bakedCsgIndex =
                compileConstructiveSolidGeometry(csg, compiledScene);
        } else {
            baked.bakedCsgIndex = -1;
        }
        compileTracingObjects(
            object->getBoundingShapes(),
            compiledScene,
            baked.boundingObjects);
        compileTracingObjects(
            object->getClippingShapes(),
            compiledScene,
            baked.clippingObjects);
        compiledScene.bakedSimpleBodies.set(entry.bakedSimpleBodyIndex, baked);
        return entry;
    }

    Composite *composite = dynamic_cast<Composite *>(object);
    if (composite != nullptr) {
        entry.bakedCompositeIndex = compiledScene.bakedComposites.size();
        // Reserve the parent slot before compiling children: nested composites
        // may append their own baked nodes during bakeComposite().
        compiledScene.bakedComposites.add(Scene::BakedComposite());
        compiledScene.bakedComposites.set(
            entry.bakedCompositeIndex,
            bakeComposite(composite, compiledScene));
    }
    return entry;
}
}

ColorRgba
Scene::blackFogColor()
{
    return ColorRgba(0.0, 0.0, 0.0, 0.0);
}

Scene::Scene() :
    viewPoint(defaultViewPoint()),
    screenHeight(0),
    screenWidth(0),
    atmosphereIor(1.0),
    antialiasThreshold(Scene::DEFAULT_ANTIALIAS_THRESHOLD),
    fogDistance(0.0),
    fogColor(0.0, 0.0, 0.0, 0.0)
{
}

Scene::~Scene()
{
    // lightSources holds non-owning pointers into the LightGeometryAdapter
    // tree already owned by Objects; the Light objects are freed via Objects.
    for (long int i = 0; i < Objects.size(); i++) {
        delete Objects[i];
    }
    delete defaultTexture;
}

void
Scene::rebuildTracingStructures()
{
    buildCompiledTracingScene();
    buildTracingCache();
}

void
Scene::buildTracingCache()
{
    if (compiledTracingScene.objects.size() != Objects.size()) {
        buildCompiledTracingScene();
    }

    boundedTracingObjects.clear();
    unboundedTracingObjects.clear();

    boundedTracingObjects.reserve(compiledTracingScene.boundedObjects.size());
    unboundedTracingObjects.reserve(compiledTracingScene.unboundedObjects.size());
    for (long int i = 0; i < compiledTracingScene.objects.size(); i++) {
        const CompiledTracingObject &compiledObject = compiledTracingScene.objects[i];
        if (compiledObject.bounded) {
            TracingObjectEntry entry;
            entry.object = compiledObject.object;
            entry.bounds = compiledObject.bounds;
            boundedTracingObjects.add(entry);
        } else {
            unboundedTracingObjects.add(compiledObject.object);
        }
    }
}

void
Scene::buildCompiledTracingScene()
{
    compiledTracingScene.objects.clear();
    compiledTracingScene.bakedSimpleBodies.clear();
    compiledTracingScene.bakedCsgs.clear();
    compiledTracingScene.bakedComposites.clear();
    compiledTracingScene.boundedObjects.clear();
    compiledTracingScene.unboundedObjects.clear();
    compiledTracingScene.shadowCastingObjects.clear();
    compiledTracingScene.boundedShadowCastingObjects.clear();
    compiledTracingScene.unboundedShadowCastingObjects.clear();

    compiledTracingScene.objects.reserve(Objects.size());
    compiledTracingScene.bakedSimpleBodies.reserve(Objects.size());
    compiledTracingScene.bakedCsgs.reserve(Objects.size());
    compiledTracingScene.bakedComposites.reserve(Objects.size());
    compiledTracingScene.boundedObjects.reserve(Objects.size());
    compiledTracingScene.unboundedObjects.reserve(Objects.size());
    compiledTracingScene.shadowCastingObjects.reserve(Objects.size());
    compiledTracingScene.boundedShadowCastingObjects.reserve(Objects.size());
    compiledTracingScene.unboundedShadowCastingObjects.reserve(Objects.size());
    for (long int i = 0; i < Objects.size(); i++) {
        SimpleBody *object = Objects[i];
        if (object == nullptr) {
            continue;
        }

        CompiledTracingObject entry =
            compileTracingObject(object, compiledTracingScene);
        compiledTracingScene.objects.add(entry);

        if (entry.bounded) {
            compiledTracingScene.boundedObjects.add(entry);
        } else {
            compiledTracingScene.unboundedObjects.add(entry);
        }
        if (entry.castsShadow) {
            compiledTracingScene.shadowCastingObjects.add(entry);
            if (entry.bounded) {
                compiledTracingScene.boundedShadowCastingObjects.add(entry);
            } else {
                compiledTracingScene.unboundedShadowCastingObjects.add(entry);
            }
        }
    }
}

void
Scene::resetForSceneParse(double antialiasThreshold)
{
    viewPoint = defaultViewPoint();
    lightSources.clear();
    Objects.clear();
    boundedTracingObjects.clear();
    unboundedTracingObjects.clear();
    compiledTracingScene.objects.clear();
    compiledTracingScene.bakedSimpleBodies.clear();
    compiledTracingScene.bakedCsgs.clear();
    compiledTracingScene.bakedComposites.clear();
    compiledTracingScene.boundedObjects.clear();
    compiledTracingScene.unboundedObjects.clear();
    compiledTracingScene.shadowCastingObjects.clear();
    compiledTracingScene.boundedShadowCastingObjects.clear();
    compiledTracingScene.unboundedShadowCastingObjects.clear();
    atmosphereIor = 1.0;
    this->antialiasThreshold = antialiasThreshold;
    setFog(blackFogColor(), 0.0);
}
