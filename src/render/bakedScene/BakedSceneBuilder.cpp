#include "java/util/ArrayList.txx"

#include "environment/scene/Composite.h"
#include "render/bakedScene/BakedSceneBuilder.h"

namespace {

// New traceableObjects index space: simple bodies occupy
// [0, compiledScene.bakedSimpleBodies.size()), composites occupy
// [that, that + bakedComposites.size()). A single extra slot at the very
// end represents an (in practice unreachable, see
// doc/performanceReviewPlan6.md Phase 0) entry with neither index set.
int
traceableIndexFor(
    const Scene::CompiledTracingObject &entry,
    long int simpleBodyCount,
    long int compositeCount)
{
    if (entry.bakedSimpleBodyIndex >= 0) {
        return (int)entry.bakedSimpleBodyIndex;
    }
    if (entry.bakedCompositeIndex >= 0) {
        return (int)(simpleBodyCount + entry.bakedCompositeIndex);
    }
    return (int)(simpleBodyCount + compositeCount);
}

void
mapIndices(
    const java::ArrayList<Scene::CompiledTracingObject> &in,
    long int simpleBodyCount,
    long int compositeCount,
    java::ArrayList<int> &out)
{
    out.clear();
    out.reserve(in.size());
    for (long int i = 0; i < in.size(); i++) {
        out.add(traceableIndexFor(in[i], simpleBodyCount, compositeCount));
    }
}

BakedScene::CsgOperandKind
translateOperandKind(Scene::BakedCsgOperandExecutionKind kind)
{
    switch (kind) {
    case Scene::BakedCsgOperandExecutionKind::Empty:
        return BakedScene::CsgOperandKind::Empty;
    case Scene::BakedCsgOperandExecutionKind::DirectAnnotatedPrimitive:
        return BakedScene::CsgOperandKind::DirectAnnotatedPrimitive;
    case Scene::BakedCsgOperandExecutionKind::DirectPrimitive:
        return BakedScene::CsgOperandKind::DirectPrimitive;
    case Scene::BakedCsgOperandExecutionKind::DirectPlane:
        return BakedScene::CsgOperandKind::DirectPlane;
    case Scene::BakedCsgOperandExecutionKind::TransformedPlane:
        return BakedScene::CsgOperandKind::TransformedPlane;
    case Scene::BakedCsgOperandExecutionKind::NestedCsg:
        return BakedScene::CsgOperandKind::NestedCsg;
    case Scene::BakedCsgOperandExecutionKind::TransformedNestedCsg:
        return BakedScene::CsgOperandKind::TransformedNestedCsg;
    case Scene::BakedCsgOperandExecutionKind::TransformedQuadric:
        return BakedScene::CsgOperandKind::TransformedQuadric;
    case Scene::BakedCsgOperandExecutionKind::TransformedSphere:
        return BakedScene::CsgOperandKind::TransformedSphere;
    case Scene::BakedCsgOperandExecutionKind::TransformedPrimitive:
        return BakedScene::CsgOperandKind::TransformedPrimitive;
    case Scene::BakedCsgOperandExecutionKind::GenericFallback:
        return BakedScene::CsgOperandKind::GenericFallback;
    }
    return BakedScene::CsgOperandKind::GenericFallback;
}

BakedScene::CsgPlanKind
translatePlanKind(Scene::BakedCsgExecutionPlanKind kind)
{
    switch (kind) {
    case Scene::BakedCsgExecutionPlanKind::GenericMorgan:
        return BakedScene::CsgPlanKind::GenericMorgan;
    case Scene::BakedCsgExecutionPlanKind::GenericRaySegments:
        return BakedScene::CsgPlanKind::GenericRaySegments;
    case Scene::BakedCsgExecutionPlanKind::TopLevelPlaneUnion:
        return BakedScene::CsgPlanKind::TopLevelPlaneUnion;
    case Scene::BakedCsgExecutionPlanKind::DisjointBoundedUnion:
        return BakedScene::CsgPlanKind::DisjointBoundedUnion;
    case Scene::BakedCsgExecutionPlanKind::SingleCorePlaneIntersection:
        return BakedScene::CsgPlanKind::SingleCorePlaneIntersection;
    case Scene::BakedCsgExecutionPlanKind::Fallback:
        return BakedScene::CsgPlanKind::Fallback;
    }
    return BakedScene::CsgPlanKind::Fallback;
}

BakedScene::CsgOperandRecord
translateOperand(const Scene::BakedCsgOperand &operand)
{
    BakedScene::CsgOperandRecord out;
    out.kind = translateOperandKind(operand.executionKind);
    out.geometry = operand.geometry;
    out.quadricGeometry = operand.quadricGeometry;
    out.material = operand.material;
    out.nestedCsgProgramIndex = operand.bakedCsgIndex;
    out.bakedBounds = operand.bakedBounds;
    out.objectToLocal = operand.objectToLocal;
    out.localToObject = operand.localToObject;
    out.hasTransform = operand.hasTransform;
    out.bounded = operand.bounded;
    out.cullSafe = operand.cullSafe;
    out.isInfinitePlane = operand.isInfinitePlane;
    out.planeNormal = operand.planeNormal;
    out.planeDistance = operand.planeDistance;
    out.bakedQuadricStorage = operand.bakedQuadric;
    out.hasBakedQuadric = operand.hasBakedQuadric;
    out.bakedPlaneStorage = operand.bakedPlane;
    out.hasBakedPlane = operand.hasBakedPlane;
    out.compiledTransformedNestedCorePlane = operand.compiledTransformedNestedCorePlane;
    out.compiledNestedCoreOperandIndex = operand.compiledNestedCoreOperandIndex;
    out.compiledNestedCoreDirectQuadric = operand.compiledNestedCoreDirectQuadric;
    out.compiledNestedCoreTransformedQuadric = operand.compiledNestedCoreTransformedQuadric;
    out.compiledNestedPlaneOperandIndices = operand.compiledNestedPlaneOperandIndices;
    out.compiledNestedContainmentOperandIndices = operand.compiledNestedContainmentOperandIndices;

    // Re-point a folded operand's owned copy at itself: `operand.geometry`/
    // `quadricGeometry` were already re-pointed at `operand.bakedQuadric`/
    // `bakedPlane` by Scene::buildCompiledTracingScene's fix-up pass, but
    // that address belongs to the OLD struct, which does not outlive this
    // function. Re-derive it against our own by-value copy instead.
    if (out.hasBakedQuadric) {
        out.geometry = &out.bakedQuadricStorage;
        out.quadricGeometry = &out.bakedQuadricStorage;
    } else if (out.hasBakedPlane) {
        out.geometry = &out.bakedPlaneStorage;
    }
    return out;
}

BakedScene::CsgProgram
translateCsgProgram(const Scene::BakedConstructiveSolidGeometry &csg)
{
    BakedScene::CsgProgram out;
    out.algorithm =
        csg.algorithm == Scene::BakedCsgAlgorithm::RaySegments ?
            BakedScene::CsgAlgorithm::RaySegments :
            BakedScene::CsgAlgorithm::MorganRules;
    out.planKind = translatePlanKind(csg.executionPlanKind);
    out.geometryType = csg.geometryType;
    out.topLevel = csg.topLevel;
    out.specializationValid = csg.specializationValid;
    out.specializationCoreOperandIndex = csg.specializationCoreOperandIndex;
    out.planeOperandIndices = csg.executionPlanPlaneOperandIndices;
    out.nestedOperandIndices = csg.executionPlanNestedOperandIndices;
    out.transformedPrimitiveOperandIndices =
        csg.executionPlanTransformedPrimitiveOperandIndices;
    out.directPrimitiveOperandIndices = csg.executionPlanDirectPrimitiveOperandIndices;

    out.operands.reserve(csg.operands.size());
    for (long int i = 0; i < csg.operands.size(); i++) {
        out.operands.add(translateOperand(csg.operands[i]));
    }
    return out;
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

BakedScene::TraceableObject
translateSimpleBody(
    const Scene::BakedSimpleBody &body,
    long int simpleBodyCount,
    long int compositeCount)
{
    BakedScene::TraceableObject out;
    out.object = body.object;
    out.geometry = body.geometry;
    out.quadricGeometry = body.quadricGeometry;
    out.geometryMaterial = body.geometryMaterial;
    out.objectTexture = body.objectTexture;
    out.objectColor = body.objectColor;
    out.worldBounds = body.worldBounds;
    out.bounded = body.bounded;
    out.noShadowFlag = body.noShadowFlag;
    out.castsShadow = !body.noShadowFlag;
    out.csgProgramIndex = body.bakedCsgIndex;
    out.compositeIndex = -1;
    out.objectToWorld = body.objectToWorld;
    out.worldToObject = body.worldToObject;
    out.geometryToObject = body.geometryToObject;
    out.objectToGeometry = body.objectToGeometry;
    out.geometryToWorld = body.geometryToWorld;
    out.worldToGeometry = body.worldToGeometry;
    out.hasObjectTransform = body.hasObjectTransform;
    out.hasGeometryTransform = body.hasGeometryTransform;
    out.hasBoundingShapes = body.hasBoundingShapes;
    out.hasClippingShapes = body.hasClippingShapes;
    mapIndices(body.boundingObjects, simpleBodyCount, compositeCount, out.boundingObjectIndices);
    mapIndices(body.clippingObjects, simpleBodyCount, compositeCount, out.clippingObjectIndices);
    out.bakedQuadricStorage = body.bakedQuadric;
    out.hasBakedQuadric = body.hasBakedQuadric;
    out.bakedPlaneStorage = body.bakedPlane;
    out.hasBakedPlane = body.hasBakedPlane;
    if (out.hasBakedQuadric) {
        out.geometry = &out.bakedQuadricStorage;
        out.quadricGeometry = &out.bakedQuadricStorage;
    } else if (out.hasBakedPlane) {
        out.geometry = &out.bakedPlaneStorage;
    }
    out.kind = classifyTraceableObject(
        out.geometry != nullptr,
        out.hasBoundingShapes || out.hasClippingShapes,
        out.csgProgramIndex >= 0);
    return out;
}

BakedScene::TraceableObject
translateCompositeAsTraceableObject(
    const Scene::BakedComposite &composite, int compositeIndex)
{
    BakedScene::TraceableObject out;
    out.kind = BakedScene::TraceKind::Composite;
    out.object = composite.object;
    out.geometry = nullptr;
    out.worldBounds = composite.worldBounds;
    out.bounded = composite.bounded;
    out.noShadowFlag = composite.noShadowFlag;
    out.castsShadow = !composite.noShadowFlag;
    out.csgProgramIndex = -1;
    out.compositeIndex = compositeIndex;
    out.objectToWorld = composite.objectToWorld;
    out.worldToObject = composite.worldToObject;
    out.hasObjectTransform = composite.hasObjectTransform;
    out.hasGeometryTransform = false;
    out.hasBoundingShapes = composite.hasBoundingShapes;
    out.hasClippingShapes = composite.hasClippingShapes;
    return out;
}

BakedScene::CompositeRecord
translateComposite(
    const Scene::BakedComposite &composite,
    long int simpleBodyCount,
    long int compositeCount)
{
    BakedScene::CompositeRecord out;
    out.object = composite.object;
    out.worldBounds = composite.worldBounds;
    out.objectToWorld = composite.objectToWorld;
    out.worldToObject = composite.worldToObject;
    out.bounded = composite.bounded;
    out.noShadowFlag = composite.noShadowFlag;
    out.hasObjectTransform = composite.hasObjectTransform;
    out.hasBoundingShapes = composite.hasBoundingShapes;
    out.hasClippingShapes = composite.hasClippingShapes;
    mapIndices(
        composite.boundingObjects, simpleBodyCount, compositeCount, out.boundingObjectIndices);
    mapIndices(
        composite.clippingObjects, simpleBodyCount, compositeCount, out.clippingObjectIndices);
    mapIndices(
        composite.childObjects, simpleBodyCount, compositeCount, out.childObjectIndices);
    return out;
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

BakedScene
BakedSceneBuilder::build(const Scene::CompiledTracingScene &compiledScene)
{
    BakedScene scene;
    const long int simpleBodyCount = compiledScene.bakedSimpleBodies.size();
    const long int compositeCount = compiledScene.bakedComposites.size();

    scene.csgPrograms.reserve(compiledScene.bakedCsgs.size());
    for (long int i = 0; i < compiledScene.bakedCsgs.size(); i++) {
        scene.csgPrograms.add(translateCsgProgram(compiledScene.bakedCsgs[i]));
    }

    scene.composites.reserve(compositeCount);
    for (long int i = 0; i < compositeCount; i++) {
        scene.composites.add(
            translateComposite(compiledScene.bakedComposites[i], simpleBodyCount, compositeCount));
    }

    scene.traceableObjects.reserve(simpleBodyCount + compositeCount + 1);
    for (long int i = 0; i < simpleBodyCount; i++) {
        scene.traceableObjects.add(
            translateSimpleBody(
                compiledScene.bakedSimpleBodies[i], simpleBodyCount, compositeCount));
    }
    for (long int i = 0; i < compositeCount; i++) {
        scene.traceableObjects.add(
            translateCompositeAsTraceableObject(compiledScene.bakedComposites[i], (int)i));
    }
    // Sentinel Empty entry for the (unreached in practice) case of a
    // CompiledTracingObject with neither baked index set - see
    // traceableIndexFor() above.
    scene.traceableObjects.add(BakedScene::TraceableObject());

    mapIndices(compiledScene.objects, simpleBodyCount, compositeCount, scene.topLevelObjectIndices);
    mapIndices(
        compiledScene.boundedObjects, simpleBodyCount, compositeCount, scene.boundedObjectIndices);
    mapIndices(
        compiledScene.unboundedObjects,
        simpleBodyCount,
        compositeCount,
        scene.unboundedObjectIndices);
    mapIndices(
        compiledScene.shadowCastingObjects,
        simpleBodyCount,
        compositeCount,
        scene.shadowCastingObjectIndices);
    mapIndices(
        compiledScene.boundedShadowCastingObjects,
        simpleBodyCount,
        compositeCount,
        scene.boundedShadowCastingObjectIndices);
    mapIndices(
        compiledScene.unboundedShadowCastingObjects,
        simpleBodyCount,
        compositeCount,
        scene.unboundedShadowCastingObjectIndices);

    accumulateStatistics(scene);
    return scene;
}
