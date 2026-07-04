#include "java/util/ArrayList.txx"

#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/scene/Composite.h"
#include "render/bakedScene/BakedGeometryBaker.h"
#include "render/bakedScene/BakedSceneBuilder.h"

namespace {

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

BakedScene::TraceKind
classifyTraceableObject(bool hasGeometry, bool boundedOrClipped, bool hasCsg)
{
    if (!hasGeometry) {
        return BakedScene::TraceKind::Empty;
    }
    if (boundedOrClipped) {
        return BakedScene::TraceKind::BoundedGeneric;
    }
    return hasCsg ? BakedScene::TraceKind::Csg : BakedScene::TraceKind::DirectPrimitive;
}

BakedScene::CsgOperandKind
classifyOperandKind(const BakedScene::CsgOperandRecord &baked)
{
    if (baked.geometry == nullptr) {
        return BakedScene::CsgOperandKind::Empty;
    }
    if (baked.nestedCsgProgramIndex >= 0) {
        return baked.hasTransform ?
            BakedScene::CsgOperandKind::TransformedNestedCsg :
            BakedScene::CsgOperandKind::NestedCsg;
    }
    if (baked.isInfinitePlane) {
        return baked.hasTransform ?
            BakedScene::CsgOperandKind::TransformedPlane :
            BakedScene::CsgOperandKind::DirectPlane;
    }
    if (baked.hasTransform) {
        if (baked.quadricGeometry != nullptr) {
            return BakedScene::CsgOperandKind::TransformedQuadric;
        }
        if (dynamic_cast<Sphere *>(baked.geometry) != nullptr) {
            return BakedScene::CsgOperandKind::TransformedSphere;
        }
        return BakedScene::CsgOperandKind::TransformedPrimitive;
    }
    return baked.geometry->hasNativeAnnotatedCrossings() ?
        BakedScene::CsgOperandKind::DirectAnnotatedPrimitive :
        BakedScene::CsgOperandKind::DirectPrimitive;
}

bool
hasFiniteInteriorBounds(const BakedScene::CsgOperandRecord &operand)
{
    if (!operand.bounded || !operand.cullSafe || operand.nestedCsgProgramIndex >= 0) {
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
    const java::ArrayList<BakedScene::CsgOperandRecord> &operands)
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

// Classifies the CSG specialization and, when one applies, writes the
// matching BakedScene::CsgPlanKind directly (buildCsgExecutionPlan below
// only fills in the non-specialized planKind values) - this folds what used
// to be a separate BakedCsgSpecialization enum into planKind itself, since
// specializationValid is only ever true together with one of these three
// specific values (see doc/performanceReviewPlan6.md Phase 3 status).
void
classifyCsgProgramSpecialization(BakedScene::CsgProgram &baked)
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
            baked.planKind = BakedScene::CsgPlanKind::TopLevelPlaneUnion;
            baked.specializationValid = true;
            return;
        }
        if (hasPairwiseDisjointFiniteOperands(baked.operands)) {
            baked.planKind = BakedScene::CsgPlanKind::DisjointBoundedUnion;
            baked.specializationValid = true;
        }
        return;
    }

    if (baked.geometryType != BooleanSetOperations::INTERSECTION ||
        baked.operands.size() < 2) {
        return;
    }

    int coreIndex = -1;
    for (long int i = 0; i < baked.operands.size(); i++) {
        const BakedScene::CsgOperandRecord &operand = baked.operands[i];
        if (operand.isInfinitePlane && operand.nestedCsgProgramIndex < 0) {
            continue;
        }
        if (coreIndex >= 0) {
            return;
        }
        coreIndex = i;
    }
    if (coreIndex >= 0) {
        baked.planKind = BakedScene::CsgPlanKind::SingleCorePlaneIntersection;
        baked.specializationCoreOperandIndex = coreIndex;
        baked.specializationValid = true;
    }
}

// `out` is read here (for already-compiled nested CsgPrograms only - a
// nested operand's underlying CSG is always compiled before this function
// runs for the outer CSG, see bakeCsgOperand/bakeConstructiveSolidGeometry
// below), never written except through `baked` itself.
void
buildCsgExecutionPlan(BakedScene::CsgProgram &baked, const BakedScene &out)
{
    baked.planeOperandIndices.clear();
    baked.nestedOperandIndices.clear();
    baked.transformedPrimitiveOperandIndices.clear();
    baked.directPrimitiveOperandIndices.clear();

    for (long int i = 0; i < baked.operands.size(); i++) {
        BakedScene::CsgOperandRecord &operand = baked.operands[i];
        operand.compiledTransformedNestedCorePlane = false;
        operand.compiledNestedCoreOperandIndex = -1;
        operand.compiledNestedCoreDirectQuadric = false;
        operand.compiledNestedCoreTransformedQuadric = false;
        operand.compiledNestedPlaneOperandIndices.clear();
        operand.compiledNestedContainmentOperandIndices.clear();
        switch (operand.kind) {
        case BakedScene::CsgOperandKind::DirectPlane:
        case BakedScene::CsgOperandKind::TransformedPlane:
            baked.planeOperandIndices.add((int)i);
            break;
        case BakedScene::CsgOperandKind::NestedCsg:
        case BakedScene::CsgOperandKind::TransformedNestedCsg:
            baked.nestedOperandIndices.add((int)i);
            if (operand.kind == BakedScene::CsgOperandKind::TransformedNestedCsg &&
                operand.nestedCsgProgramIndex >= 0 &&
                operand.nestedCsgProgramIndex < out.csgPrograms.size()) {
                const BakedScene::CsgProgram &nestedCsg =
                    out.csgPrograms[operand.nestedCsgProgramIndex];
                const int coreIndex = nestedCsg.specializationCoreOperandIndex;
                if (nestedCsg.planKind ==
                        BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
                    nestedCsg.specializationValid &&
                    coreIndex >= 0 &&
                    coreIndex < nestedCsg.operands.size() &&
                    nestedCsg.planeOperandIndices.size() + 1 ==
                        nestedCsg.operands.size()) {
                    const BakedScene::CsgOperandRecord &coreOperand =
                        nestedCsg.operands[coreIndex];
                    const bool directQuadric =
                        coreOperand.kind ==
                            BakedScene::CsgOperandKind::DirectAnnotatedPrimitive &&
                        coreOperand.quadricGeometry != nullptr;
                    const bool transformedQuadric =
                        coreOperand.kind ==
                            BakedScene::CsgOperandKind::TransformedQuadric &&
                        coreOperand.quadricGeometry != nullptr;
                    if (directQuadric || transformedQuadric) {
                        operand.compiledTransformedNestedCorePlane = true;
                        operand.compiledNestedCoreOperandIndex = coreIndex;
                        operand.compiledNestedCoreDirectQuadric = directQuadric;
                        operand.compiledNestedCoreTransformedQuadric = transformedQuadric;
                        operand.compiledNestedPlaneOperandIndices.reserve(
                            nestedCsg.planeOperandIndices.size());
                        operand.compiledNestedContainmentOperandIndices.reserve(
                            nestedCsg.planeOperandIndices.size() + 1);
                        operand.compiledNestedContainmentOperandIndices.add(coreIndex);
                        for (long int p = 0;
                             p < nestedCsg.planeOperandIndices.size();
                             p++) {
                            const int planeOperandIndex =
                                nestedCsg.planeOperandIndices[p];
                            operand.compiledNestedPlaneOperandIndices.add(
                                planeOperandIndex);
                            operand.compiledNestedContainmentOperandIndices.add(
                                planeOperandIndex);
                        }
                    }
                }
            }
            break;
        case BakedScene::CsgOperandKind::TransformedQuadric:
        case BakedScene::CsgOperandKind::TransformedSphere:
        case BakedScene::CsgOperandKind::TransformedPrimitive:
            baked.transformedPrimitiveOperandIndices.add((int)i);
            break;
        case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
        case BakedScene::CsgOperandKind::DirectPrimitive:
            baked.directPrimitiveOperandIndices.add((int)i);
            break;
        case BakedScene::CsgOperandKind::Empty:
        case BakedScene::CsgOperandKind::GenericFallback:
            break;
        }
    }

    if (!baked.specializationValid) {
        baked.planKind =
            baked.algorithm == BakedScene::CsgAlgorithm::RaySegments ?
                BakedScene::CsgPlanKind::GenericRaySegments :
                BakedScene::CsgPlanKind::GenericMorgan;
    }
}

int compileTracingObject(SimpleBody *object, BakedScene &out);
int compileConstructiveSolidGeometry(ConstructiveSolidGeometry *geometry, BakedScene &out);

void
compileTracingObjects(
    const java::ArrayList<SimpleBody*> &objects,
    BakedScene &out,
    java::ArrayList<int> &result)
{
    result.clear();
    result.reserve(objects.size());
    for (long int i = 0; i < objects.size(); i++) {
        if (objects[i] == nullptr) {
            continue;
        }
        result.add(compileTracingObject(objects[i], out));
    }
}

// Ported from Scene.cpp's bakeSimpleBody (pre-Phase-4). Deliberately does
// NOT re-point `geometry`/`quadricGeometry` at `bakedQuadricStorage`/
// `bakedPlaneStorage` when a fold succeeds - both are local-to-this-call
// storage about to be copied into `out.traceableObjects` via add()/set(),
// so any self-pointer set here would dangle the moment that copy happens
// (see doc/performanceReviewPlan6.md Phase 2's self-referential-pointer
// bug). The one-time fix-up runs in BakedSceneBuilder::build, once
// `out.traceableObjects` has stopped growing for good.
BakedScene::TraceableObject
bakeSimpleBody(SimpleBody *object, BakedScene &out)
{
    BakedScene::TraceableObject baked;
    baked.object = object;
    baked.geometry = object->getGeometry();
    baked.quadricGeometry = dynamic_cast<Quadric *>(baked.geometry);
    baked.geometryMaterial = object->getGeometryMaterial();
    baked.objectTexture = object->getObjectTexture();
    baked.objectColor = object->getObjectColor();
    baked.worldBounds = object->getAABB();
    baked.bounded = !baked.worldBounds.isUnbounded();
    baked.noShadowFlag = object->getNoShadowFlag();
    baked.castsShadow = !baked.noShadowFlag;
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

    // Plan 5 Phase 4 fold (unchanged from the pre-Phase-4 Scene.cpp version):
    // gated on no bounding/clipping shapes, since those are tested against
    // the object-space ray/point keyed off hasObjectTransform/
    // hasGeometryTransform - clearing them would feed those tests a
    // world-space ray/point against object-local bounding geometry.
    if (!baked.hasBoundingShapes && !baked.hasClippingShapes &&
        (baked.hasObjectTransform || baked.hasGeometryTransform)) {
        java::ArrayList<TransformStep> combinedSteps(
            object->getGeometrySteps().size() + object->getBodySteps().size());
        for (long int i = 0; i < object->getGeometrySteps().size(); i++) {
            combinedSteps.add(object->getGeometrySteps()[i]);
        }
        for (long int i = 0; i < object->getBodySteps().size(); i++) {
            combinedSteps.add(object->getBodySteps()[i]);
        }
        if (combinedSteps.size() > 0) {
            if (baked.quadricGeometry != nullptr) {
                baked.bakedQuadricStorage =
                    BakedGeometryBaker::bakeQuadric(*baked.quadricGeometry, combinedSteps);
                baked.hasBakedQuadric = true;
                baked.hasObjectTransform = false;
                baked.hasGeometryTransform = false;
                object->setBakedTransformFolded(true);
            } else if (InfinitePlane *plane = dynamic_cast<InfinitePlane *>(baked.geometry)) {
                baked.bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, combinedSteps);
                baked.hasBakedPlane = true;
                baked.hasObjectTransform = false;
                baked.hasGeometryTransform = false;
                object->setBakedTransformFolded(true);
            }
        }
    }

    if (ConstructiveSolidGeometry *csg =
            dynamic_cast<ConstructiveSolidGeometry *>(baked.geometry)) {
        baked.csgProgramIndex = compileConstructiveSolidGeometry(csg, out);
    } else {
        baked.csgProgramIndex = -1;
    }

    return baked;
}

// Ported from Scene.cpp's bakeCsgOperand (pre-Phase-4); see bakeSimpleBody's
// comment above re: not re-pointing geometry/quadricGeometry here.
BakedScene::CsgOperandRecord
bakeCsgOperand(CsgOperand *operand, BakedScene &out)
{
    BakedScene::CsgOperandRecord baked;
    baked.operand = operand;
    baked.geometry = operand->getGeometry();
    baked.material = operand->getMaterial();
    baked.hasTransform = operand->getTransformation() != nullptr;
    baked.objectToLocal = copyOrIdentity(operand->getTransformation());
    baked.localToObject = copyOrIdentity(operand->getTransformationInverse());
    baked.bakedBounds = operand->getBakedBounds();
    baked.bounded = operand->hasBoundedBakedBounds();
    baked.isInfinitePlane = dynamic_cast<InfinitePlane *>(baked.geometry) != nullptr;
    baked.quadricGeometry = dynamic_cast<Quadric *>(baked.geometry);
    if (InfinitePlane *plane = dynamic_cast<InfinitePlane *>(baked.geometry)) {
        baked.planeNormal = plane->getNormalVector();
        baked.planeDistance = plane->getDistance();
    }

    // Plan 5 Phase 3 collapse (unchanged) - see doc/performanceReviewPlan5.md
    // for the elementary-step congruence rationale and the double-invert bug
    // this gate's history is tied to.
    constexpr bool kEnableCoefficientCollapse = true;
    if (kEnableCoefficientCollapse &&
        baked.hasTransform && operand->getSteps().size() > 0) {
        if (baked.quadricGeometry != nullptr) {
            baked.bakedQuadricStorage =
                BakedGeometryBaker::bakeQuadric(*baked.quadricGeometry, operand->getSteps());
            baked.hasBakedQuadric = true;
            baked.hasTransform = false;
            operand->setBakedTransformFolded(true);
        } else if (baked.isInfinitePlane) {
            InfinitePlane *plane = static_cast<InfinitePlane *>(baked.geometry);
            baked.bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, operand->getSteps());
            baked.hasBakedPlane = true;
            baked.hasTransform = false;
            baked.planeNormal = baked.bakedPlaneStorage.getNormalVector();
            baked.planeDistance = baked.bakedPlaneStorage.getDistance();
            operand->setBakedTransformFolded(true);
        }
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
        baked.nestedCsgProgramIndex = compileConstructiveSolidGeometry(nestedCsg, out);
        baked.cullSafe = false;
        baked.isInfinitePlane = false;
    }
    baked.kind = classifyOperandKind(baked);
    return baked;
}

BakedScene::CsgProgram
bakeConstructiveSolidGeometry(ConstructiveSolidGeometry *geometry, BakedScene &out)
{
    BakedScene::CsgProgram baked;
    baked.geometryType = geometry->getGeometryType();
    if (ConstructiveSolidGeometryByRaySegment *raySegment =
            dynamic_cast<ConstructiveSolidGeometryByRaySegment *>(geometry)) {
        baked.algorithm = BakedScene::CsgAlgorithm::RaySegments;
        baked.topLevel = raySegment->isTopLevel();
    } else {
        baked.algorithm = BakedScene::CsgAlgorithm::MorganRules;
        baked.topLevel = false;
    }

    const java::ArrayList<CsgOperand*> &operands = geometry->getOperands();
    baked.operands.reserve(operands.size());
    for (long int i = 0; i < operands.size(); i++) {
        if (operands[i] == nullptr) {
            continue;
        }
        baked.operands.add(bakeCsgOperand(operands[i], out));
    }
    classifyCsgProgramSpecialization(baked);
    buildCsgExecutionPlan(baked, out);
    return baked;
}

int
compileConstructiveSolidGeometry(ConstructiveSolidGeometry *geometry, BakedScene &out)
{
    const int index = out.csgPrograms.size();
    out.csgPrograms.add(BakedScene::CsgProgram());
    out.csgPrograms.set(index, bakeConstructiveSolidGeometry(geometry, out));
    return index;
}

BakedScene::CompositeRecord
bakeComposite(Composite *object, BakedScene &out)
{
    BakedScene::CompositeRecord baked;
    baked.object = object;
    baked.worldBounds = object->getAABB();
    baked.bounded = !baked.worldBounds.isUnbounded();
    baked.noShadowFlag = object->getNoShadowFlag();
    baked.hasObjectTransform = object->getTransformation() != nullptr;
    baked.hasBoundingShapes = object->getBoundingShapes().size() > 0;
    baked.hasClippingShapes = object->getClippingShapes().size() > 0;
    baked.objectToWorld = copyOrIdentity(object->getTransformation());
    baked.worldToObject = copyOrIdentity(object->getTransformationInverse());

    compileTracingObjects(object->getBoundingShapes(), out, baked.boundingObjectIndices);
    compileTracingObjects(object->getClippingShapes(), out, baked.clippingObjectIndices);

    const java::ArrayList<SimpleBody*> &children = object->getSimpleBodies();
    baked.childObjectIndices.reserve(children.size());
    for (long int i = 0; i < children.size(); i++) {
        baked.childObjectIndices.add(compileTracingObject(children[i], out));
    }
    return baked;
}

// Ported from Scene.cpp's compileTracingObject. Returns the new object's
// index in `out.traceableObjects` directly (the old CompiledTracingObject
// return value - a small bounded/castsShadow/dispatch-index summary - is
// gone; callers just re-index `out.traceableObjects[index]` for those
// fields, which is safe: array growth reallocates the underlying storage,
// never the logical index).
int
compileTracingObject(SimpleBody *object, BakedScene &out)
{
    const int index = out.traceableObjects.size();
    out.traceableObjects.add(BakedScene::TraceableObject());

    if (isBakeableSimpleBody(object)) {
        BakedScene::TraceableObject baked = bakeSimpleBody(object, out);
        compileTracingObjects(object->getBoundingShapes(), out, baked.boundingObjectIndices);
        compileTracingObjects(object->getClippingShapes(), out, baked.clippingObjectIndices);
        baked.kind = classifyTraceableObject(
            baked.geometry != nullptr,
            baked.hasBoundingShapes || baked.hasClippingShapes,
            baked.csgProgramIndex >= 0);
        out.traceableObjects.set(index, baked);
        return index;
    }

    Composite *composite = dynamic_cast<Composite *>(object);
    if (composite != nullptr) {
        const int compositeIndex = out.composites.size();
        // Reserve the parent slot before compiling children: nested
        // composites may append their own baked nodes during bakeComposite().
        out.composites.add(BakedScene::CompositeRecord());
        BakedScene::CompositeRecord bakedComposite = bakeComposite(composite, out);
        out.composites.set(compositeIndex, bakedComposite);

        BakedScene::TraceableObject entry;
        entry.kind = BakedScene::TraceKind::Composite;
        entry.object = composite;
        entry.geometry = nullptr;
        entry.worldBounds = bakedComposite.worldBounds;
        entry.bounded = bakedComposite.bounded;
        entry.noShadowFlag = bakedComposite.noShadowFlag;
        entry.castsShadow = !bakedComposite.noShadowFlag;
        entry.csgProgramIndex = -1;
        entry.compositeIndex = compositeIndex;
        entry.objectToWorld = bakedComposite.objectToWorld;
        entry.worldToObject = bakedComposite.worldToObject;
        entry.hasObjectTransform = bakedComposite.hasObjectTransform;
        entry.hasGeometryTransform = false;
        entry.hasBoundingShapes = bakedComposite.hasBoundingShapes;
        entry.hasClippingShapes = bakedComposite.hasClippingShapes;
        out.traceableObjects.set(index, entry);
        return index;
    }

    return index;
}

void
accumulateStatistics(BakedScene &scene)
{
    BakedScene::Statistics &stats = scene.statistics;
    for (long int i = 0; i < scene.traceableObjects.size(); i++) {
        const int kindIndex = (int)scene.traceableObjects[i].kind;
        if (kindIndex >= 0 && kindIndex < 6) {
            stats.countByKind[kindIndex]++;
        }
    }

    stats.csgProgramCount = scene.csgPrograms.size();
    for (long int i = 0; i < scene.csgPrograms.size(); i++) {
        const BakedScene::CsgProgram &program = scene.csgPrograms[i];
        switch (program.planKind) {
        case BakedScene::CsgPlanKind::TopLevelPlaneUnion:
            stats.csgPlanTopLevelPlaneUnion++;
            break;
        case BakedScene::CsgPlanKind::DisjointBoundedUnion:
            stats.csgPlanDisjointBoundedUnion++;
            break;
        case BakedScene::CsgPlanKind::SingleCorePlaneIntersection:
            stats.csgPlanSingleCorePlaneIntersection++;
            break;
        case BakedScene::CsgPlanKind::GenericMorgan:
            stats.csgPlanGenericMorgan++;
            break;
        case BakedScene::CsgPlanKind::GenericRaySegments:
            stats.csgPlanGenericRaySegments++;
            break;
        case BakedScene::CsgPlanKind::Fallback:
            stats.csgPlanFallback++;
            break;
        }
        for (long int j = 0; j < program.operands.size(); j++) {
            const BakedScene::CsgOperandRecord &operand = program.operands[j];
            if (operand.hasBakedQuadric) {
                stats.residualBakedQuadricOperands++;
            }
            if (operand.hasBakedPlane) {
                stats.residualBakedPlaneOperands++;
            }
            if (operand.hasTransform) {
                stats.residualTransformedOperands++;
            }
        }
    }
}

}

void
BakedSceneBuilder::build(const java::ArrayList<SimpleBody*> &objects, BakedScene &out)
{
    out.traceableObjects.clear();
    out.csgPrograms.clear();
    out.composites.clear();
    out.topLevelObjectIndices.clear();
    out.boundedObjectIndices.clear();
    out.unboundedObjectIndices.clear();
    out.shadowCastingObjectIndices.clear();
    out.boundedShadowCastingObjectIndices.clear();
    out.unboundedShadowCastingObjectIndices.clear();
    out.statistics = BakedScene::Statistics();

    out.traceableObjects.reserve(objects.size());
    out.csgPrograms.reserve(objects.size());
    out.composites.reserve(objects.size());
    out.topLevelObjectIndices.reserve(objects.size());
    out.boundedObjectIndices.reserve(objects.size());
    out.unboundedObjectIndices.reserve(objects.size());
    out.shadowCastingObjectIndices.reserve(objects.size());
    out.boundedShadowCastingObjectIndices.reserve(objects.size());
    out.unboundedShadowCastingObjectIndices.reserve(objects.size());

    for (long int i = 0; i < objects.size(); i++) {
        SimpleBody *object = objects[i];
        if (object == nullptr) {
            continue;
        }

        const int index = compileTracingObject(object, out);
        out.topLevelObjectIndices.add(index);

        const BakedScene::TraceableObject &entry = out.traceableObjects[index];
        if (entry.bounded) {
            out.boundedObjectIndices.add(index);
        } else {
            out.unboundedObjectIndices.add(index);
        }
        if (entry.castsShadow) {
            out.shadowCastingObjectIndices.add(index);
            if (entry.bounded) {
                out.boundedShadowCastingObjectIndices.add(index);
            } else {
                out.unboundedShadowCastingObjectIndices.add(index);
            }
        }
    }

    // One-time fix-up of `geometry`/`quadricGeometry` for operands/objects
    // collapsed to a baked (world-space) Quadric/InfinitePlane copy. MUST
    // run after every add()/set() above has finished: the bake pipeline
    // copies/reallocates these arrays several times on the way here
    // (ArrayList growth and .set() both copy-assign, per ArrayList.txx), so
    // a self-pointer set any earlier would dangle. From this point on
    // `out.csgPrograms`/`out.traceableObjects` never grow or reallocate
    // again until the next build() call, so `&operand.bakedQuadricStorage`/
    // `&obj.bakedPlaneStorage` are stable for the rest of the render.
    for (long int i = 0; i < out.csgPrograms.size(); i++) {
        BakedScene::CsgProgram &program = out.csgPrograms[i];
        for (long int j = 0; j < program.operands.size(); j++) {
            BakedScene::CsgOperandRecord &operand = program.operands[j];
            if (operand.hasBakedQuadric) {
                operand.geometry = &operand.bakedQuadricStorage;
                operand.quadricGeometry = &operand.bakedQuadricStorage;
            } else if (operand.hasBakedPlane) {
                operand.geometry = &operand.bakedPlaneStorage;
            }
        }
    }
    for (long int i = 0; i < out.traceableObjects.size(); i++) {
        BakedScene::TraceableObject &obj = out.traceableObjects[i];
        if (obj.hasBakedQuadric) {
            obj.geometry = &obj.bakedQuadricStorage;
            obj.quadricGeometry = &obj.bakedQuadricStorage;
        } else if (obj.hasBakedPlane) {
            obj.geometry = &obj.bakedPlaneStorage;
        }
    }

    // Plan 7: assign RaySharedCache slot indices, one per quadric/plane
    // record that BakedCsgTrace's viewpoint-constant helpers will query.
    // Must run after the fixup loops above so quadricGeometry is final.
    int nextQuadricSlot = 0;
    int nextPlaneSlot = 0;
    for (long int i = 0; i < out.csgPrograms.size(); i++) {
        BakedScene::CsgProgram &program = out.csgPrograms[i];
        for (long int j = 0; j < program.operands.size(); j++) {
            BakedScene::CsgOperandRecord &operand = program.operands[j];
            if (operand.quadricGeometry != nullptr) {
                operand.quadricViewpointSlot = nextQuadricSlot++;
            }
            if (operand.kind == BakedScene::CsgOperandKind::DirectPlane ||
                operand.kind == BakedScene::CsgOperandKind::TransformedPlane) {
                operand.planeViewpointSlot = nextPlaneSlot++;
            }
        }
    }
    for (long int i = 0; i < out.traceableObjects.size(); i++) {
        BakedScene::TraceableObject &obj = out.traceableObjects[i];
        if (obj.quadricGeometry != nullptr) {
            obj.quadricViewpointSlot = nextQuadricSlot++;
        }
    }
    out.statistics.quadricViewpointSlotCount = nextQuadricSlot;
    out.statistics.planeViewpointSlotCount = nextPlaneSlot;

    accumulateStatistics(out);
}
