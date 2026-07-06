

#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "environment/scene/Composite.h"
#include "render/bakedScene/BakedGeometryBaker.h"
#include "render/bakedScene/BakedSceneBuilder.h"
#include "java/util/ArrayList.txx"

bool
BakedSceneBuilder::isBakeableSimpleBody(SimpleBody *object)
{
    if (object == nullptr) {
        return false;
    }
    return dynamic_cast<Composite *>(object) == nullptr;
}

Matrix4x4d
BakedSceneBuilder::copyOrIdentity(const Matrix4x4d *matrix)
{
    return matrix != nullptr ? Matrix4x4d(*matrix) : Matrix4x4d::identityMatrix();
}

BakedSceneTraceKind
BakedSceneBuilder::classifyTraceableObject(bool hasGeometry, bool boundedOrClipped, bool hasCsg)
{
    if (!hasGeometry) {
        return BakedSceneTraceKind::Empty;
    }
    if (boundedOrClipped) {
        return BakedSceneTraceKind::BoundedGeneric;
    }
    return hasCsg ? BakedSceneTraceKind::Csg : BakedSceneTraceKind::DirectPrimitive;
}

BakedSceneCsgOperandKind
BakedSceneBuilder::classifyOperandKind(
    Geometry *geometry,
    int nestedCsgProgramIndex,
    bool hasTransform,
    bool isInfinitePlane,
    Quadric *quadricGeometry)
{
    if (geometry == nullptr) {
        return BakedSceneCsgOperandKind::Empty;
    }
    if (nestedCsgProgramIndex >= 0) {
        return hasTransform ?
            BakedSceneCsgOperandKind::TransformedNestedCsg :
            BakedSceneCsgOperandKind::NestedCsg;
    }
    if (isInfinitePlane) {
        return hasTransform ?
            BakedSceneCsgOperandKind::TransformedPlane :
            BakedSceneCsgOperandKind::DirectPlane;
    }
    if (hasTransform) {
        if (quadricGeometry != nullptr) {
            return BakedSceneCsgOperandKind::TransformedQuadric;
        }
        if (dynamic_cast<Sphere *>(geometry) != nullptr) {
            return BakedSceneCsgOperandKind::TransformedSphere;
        }
        return BakedSceneCsgOperandKind::TransformedPrimitive;
    }
    return geometry->hasNativeAnnotatedCrossings() ?
        BakedSceneCsgOperandKind::DirectAnnotatedPrimitive :
        BakedSceneCsgOperandKind::DirectPrimitive;
}

bool
BakedSceneBuilder::hasFiniteInteriorBounds(const CsgOperandRecord *operand)
{
    if (!operand->getBounded() || !operand->getCullSafe() || operand->getNestedCsgProgramIndex() >= 0) {
        return false;
    }

    const Geometry *geometry = operand->getGeometry();
    if (const Sphere *sphere = dynamic_cast<const Sphere *>(geometry)) {
        return !sphere->isInverted();
    }
    if (const Box *box = dynamic_cast<const Box *>(geometry)) {
        return !box->isInverted();
    }
    return false;
}

bool
BakedSceneBuilder::areSeparated(const AxisAlignedBoundingBox &left, const AxisAlignedBoundingBox &right)
{
    return
        left.max.x() < right.min.x() || right.max.x() < left.min.x() ||
        left.max.y() < right.min.y() || right.max.y() < left.min.y() ||
        left.max.z() < right.min.z() || right.max.z() < left.min.z();
}

bool
BakedSceneBuilder::hasPairwiseDisjointFiniteOperands(
    const java::ArrayList<CsgOperandRecord *> &operands)
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
            if (!areSeparated(operands[i]->getBakedBounds(), operands[j]->getBakedBounds())) {
                return false;
            }
        }
    }
    return true;
}

void
BakedSceneBuilder::classifyCsgProgramSpecialization(
    BooleanSetOperations geometryType,
    bool topLevel,
    const java::ArrayList<CsgOperandRecord *> &operands,
    BakedSceneCsgPlanKind &planKind,
    bool &specializationValid,
    int &specializationCoreOperandIndex)
{
    if (geometryType != BooleanSetOperations::DIFFERENCE &&
        geometryType != BooleanSetOperations::INTERSECTION) {
        bool allPlanes = operands.size() > 0;
        for (long int i = 0; i < operands.size(); i++) {
            if (!operands[i]->getIsInfinitePlane()) {
                allPlanes = false;
            }
        }
        if (topLevel && allPlanes) {
            planKind = BakedSceneCsgPlanKind::TopLevelPlaneUnion;
            specializationValid = true;
            return;
        }
        if (hasPairwiseDisjointFiniteOperands(operands)) {
            planKind = BakedSceneCsgPlanKind::DisjointBoundedUnion;
            specializationValid = true;
        }
        return;
    }

    if (geometryType != BooleanSetOperations::INTERSECTION || operands.size() < 2) {
        return;
    }

    int coreIndex = -1;
    for (long int i = 0; i < operands.size(); i++) {
        const CsgOperandRecord *operand = operands[i];
        if (operand->getIsInfinitePlane() && operand->getNestedCsgProgramIndex() < 0) {
            continue;
        }
        if (coreIndex >= 0) {
            return;
        }
        coreIndex = i;
    }
    if (coreIndex >= 0) {
        planKind = BakedSceneCsgPlanKind::SingleCorePlaneIntersection;
        specializationCoreOperandIndex = coreIndex;
        specializationValid = true;
    }
}

// `out` is read here (for already-compiled nested CsgPrograms only - a
// nested operand's underlying CSG is always compiled before this function
// runs for the outer CSG, see bakeCsgOperand/bakeConstructiveSolidGeometry
// below), never written except through `operands` itself.
void
BakedSceneBuilder::buildCsgExecutionPlan(
    BakedSceneCsgAlgorithm algorithm,
    bool specializationValid,
    BakedSceneCsgPlanKind &planKind,
    java::ArrayList<CsgOperandRecord *> &operands,
    const BakedScene &out,
    java::ArrayList<int> &planeOperandIndices,
    java::ArrayList<int> &nestedOperandIndices,
    java::ArrayList<int> &transformedPrimitiveOperandIndices,
    java::ArrayList<int> &directPrimitiveOperandIndices)
{
    planeOperandIndices.clear();
    nestedOperandIndices.clear();
    transformedPrimitiveOperandIndices.clear();
    directPrimitiveOperandIndices.clear();

    for (long int i = 0; i < operands.size(); i++) {
        CsgOperandRecord *operand = operands[i];
        bool compiledTransformedNestedCorePlane = false;
        int compiledNestedCoreOperandIndex = -1;
        bool compiledNestedCoreDirectQuadric = false;
        bool compiledNestedCoreTransformedQuadric = false;
        java::ArrayList<int> compiledNestedPlaneOperandIndices;
        java::ArrayList<int> compiledNestedContainmentOperandIndices;

        switch (operand->getKind()) {
        case BakedSceneCsgOperandKind::DirectPlane:
        case BakedSceneCsgOperandKind::TransformedPlane:
            planeOperandIndices.add((int)i);
            break;
        case BakedSceneCsgOperandKind::NestedCsg:
        case BakedSceneCsgOperandKind::TransformedNestedCsg:
            nestedOperandIndices.add((int)i);
            if ((operand->getKind() == BakedSceneCsgOperandKind::TransformedNestedCsg ||
                 (operand->getKind() == BakedSceneCsgOperandKind::NestedCsg &&
                  operand->getPushdownFolded())) &&
                operand->getNestedCsgProgramIndex() >= 0 &&
                operand->getNestedCsgProgramIndex() < out.csgPrograms.size()) {
                const CsgProgram *nestedCsg = out.csgPrograms[operand->getNestedCsgProgramIndex()];
                const int coreIndex = nestedCsg->getSpecializationCoreOperandIndex();
                if (nestedCsg->getPlanKind() ==
                        BakedSceneCsgPlanKind::SingleCorePlaneIntersection &&
                    nestedCsg->getSpecializationValid() &&
                    coreIndex >= 0 &&
                    coreIndex < nestedCsg->getOperands().size() &&
                    nestedCsg->getPlaneOperandIndices().size() + 1 ==
                        nestedCsg->getOperands().size()) {
                    const CsgOperandRecord *coreOperand = nestedCsg->getOperands()[coreIndex];
                    const bool directQuadric =
                        coreOperand->getKind() ==
                            BakedSceneCsgOperandKind::DirectAnnotatedPrimitive &&
                        coreOperand->getQuadricGeometry() != nullptr;
                    const bool transformedQuadric =
                        coreOperand->getKind() ==
                            BakedSceneCsgOperandKind::TransformedQuadric &&
                        coreOperand->getQuadricGeometry() != nullptr;
                    if (directQuadric || transformedQuadric) {
                        compiledTransformedNestedCorePlane = true;
                        compiledNestedCoreOperandIndex = coreIndex;
                        compiledNestedCoreDirectQuadric = directQuadric;
                        compiledNestedCoreTransformedQuadric = transformedQuadric;
                        compiledNestedPlaneOperandIndices.reserve(
                            nestedCsg->getPlaneOperandIndices().size());
                        compiledNestedContainmentOperandIndices.reserve(
                            nestedCsg->getPlaneOperandIndices().size() + 1);
                        compiledNestedContainmentOperandIndices.add(coreIndex);
                        for (long int p = 0;
                             p < nestedCsg->getPlaneOperandIndices().size();
                             p++) {
                            const int planeOperandIndex =
                                nestedCsg->getPlaneOperandIndices()[p];
                            compiledNestedPlaneOperandIndices.add(planeOperandIndex);
                            compiledNestedContainmentOperandIndices.add(planeOperandIndex);
                        }
                    }
                }
            }
            break;
        case BakedSceneCsgOperandKind::TransformedQuadric:
        case BakedSceneCsgOperandKind::TransformedSphere:
        case BakedSceneCsgOperandKind::TransformedPrimitive:
            transformedPrimitiveOperandIndices.add((int)i);
            break;
        case BakedSceneCsgOperandKind::DirectAnnotatedPrimitive:
        case BakedSceneCsgOperandKind::DirectPrimitive:
            directPrimitiveOperandIndices.add((int)i);
            break;
        case BakedSceneCsgOperandKind::Empty:
        case BakedSceneCsgOperandKind::GenericFallback:
            break;
        }

        operands[i] = cloneOperandWithCompiledPlan(
            operand,
            compiledTransformedNestedCorePlane,
            compiledNestedCoreOperandIndex,
            compiledNestedCoreDirectQuadric,
            compiledNestedCoreTransformedQuadric,
            compiledNestedPlaneOperandIndices,
            compiledNestedContainmentOperandIndices);
        delete operand;
    }

    if (!specializationValid) {
        planKind =
            algorithm == BakedSceneCsgAlgorithm::RaySegments ?
                BakedSceneCsgPlanKind::GenericRaySegments :
                BakedSceneCsgPlanKind::GenericMorgan;
    }
}

CsgOperandRecord *
BakedSceneBuilder::cloneOperandWithCompiledPlan(
    const CsgOperandRecord *base,
    bool compiledTransformedNestedCorePlane,
    int compiledNestedCoreOperandIndex,
    bool compiledNestedCoreDirectQuadric,
    bool compiledNestedCoreTransformedQuadric,
    const java::ArrayList<int> &compiledNestedPlaneOperandIndices,
    const java::ArrayList<int> &compiledNestedContainmentOperandIndices)
{
    return new CsgOperandRecord(
        base->getKind(), base->getOperand(), base->getOriginalGeometry(),
        base->getOriginalQuadricGeometry(), base->getMaterial(), base->getNestedCsgProgramIndex(),
        base->getBakedBounds(), base->getObjectToLocal(), base->getLocalToObject(),
        base->getHasTransform(), base->getBounded(), base->getCullSafe(), base->getIsInfinitePlane(),
        base->getPlaneNormal(), base->getPlaneDistance(), base->getQuadricViewpointSlot(),
        base->getPlaneViewpointSlot(), base->getBakedQuadricStorage(), base->getHasBakedQuadric(),
        base->getBakedPlaneStorage(), base->getHasBakedPlane(),
        compiledTransformedNestedCorePlane, compiledNestedCoreOperandIndex,
        compiledNestedCoreDirectQuadric, compiledNestedCoreTransformedQuadric,
        compiledNestedPlaneOperandIndices, compiledNestedContainmentOperandIndices,
        base->getPushdownAccumulatedSteps(), base->getPushdownFolded());
}

CsgOperandRecord *
BakedSceneBuilder::cloneOperandWithViewpointSlots(
    const CsgOperandRecord *base, int quadricViewpointSlot, int planeViewpointSlot)
{
    return new CsgOperandRecord(
        base->getKind(), base->getOperand(), base->getOriginalGeometry(),
        base->getOriginalQuadricGeometry(), base->getMaterial(), base->getNestedCsgProgramIndex(),
        base->getBakedBounds(), base->getObjectToLocal(), base->getLocalToObject(),
        base->getHasTransform(), base->getBounded(), base->getCullSafe(), base->getIsInfinitePlane(),
        base->getPlaneNormal(), base->getPlaneDistance(), quadricViewpointSlot, planeViewpointSlot,
        base->getBakedQuadricStorage(), base->getHasBakedQuadric(), base->getBakedPlaneStorage(),
        base->getHasBakedPlane(), base->getCompiledTransformedNestedCorePlane(),
        base->getCompiledNestedCoreOperandIndex(), base->getCompiledNestedCoreDirectQuadric(),
        base->getCompiledNestedCoreTransformedQuadric(), base->getCompiledNestedPlaneOperandIndices(),
        base->getCompiledNestedContainmentOperandIndices(), base->getPushdownAccumulatedSteps(),
        base->getPushdownFolded());
}

CsgOperandRecord *
BakedSceneBuilder::cloneOperandWithPushdownBake(
    const CsgOperandRecord *base,
    const AxisAlignedBoundingBox &bakedBounds,
    bool bounded,
    BakedSceneCsgOperandKind kind,
    bool hasTransform,
    const Matrix4x4d &objectToLocal,
    const Matrix4x4d &localToObject,
    bool pushdownFolded,
    const Quadric &bakedQuadricStorage,
    bool hasBakedQuadric,
    const InfinitePlane &bakedPlaneStorage,
    bool hasBakedPlane,
    const Vector3Dd &planeNormal,
    double planeDistance,
    const java::ArrayList<TransformStep> &pushdownAccumulatedSteps)
{
    return new CsgOperandRecord(
        kind, base->getOperand(), base->getOriginalGeometry(), base->getOriginalQuadricGeometry(),
        base->getMaterial(), base->getNestedCsgProgramIndex(), bakedBounds, objectToLocal, localToObject,
        hasTransform, bounded, base->getCullSafe(), base->getIsInfinitePlane(), planeNormal, planeDistance,
        base->getQuadricViewpointSlot(), base->getPlaneViewpointSlot(), bakedQuadricStorage, hasBakedQuadric,
        bakedPlaneStorage, hasBakedPlane,
        base->getCompiledTransformedNestedCorePlane(), base->getCompiledNestedCoreOperandIndex(),
        base->getCompiledNestedCoreDirectQuadric(), base->getCompiledNestedCoreTransformedQuadric(),
        base->getCompiledNestedPlaneOperandIndices(), base->getCompiledNestedContainmentOperandIndices(),
        pushdownAccumulatedSteps, pushdownFolded);
}

TraceableObject *
BakedSceneBuilder::cloneObjectWithViewpointSlot(const TraceableObject *base, int quadricViewpointSlot)
{
    return new TraceableObject(
        base->getKind(), base->getObject(), base->getOriginalGeometry(),
        base->getOriginalQuadricGeometry(), base->getGeometryMaterial(), base->getObjectTexture(),
        base->getObjectColor(), base->getWorldBounds(), base->getBounded(), base->getCastsShadow(),
        base->getNoShadowFlag(), base->getCsgProgramIndex(), base->getCompositeIndex(),
        base->getObjectToWorld(), base->getWorldToObject(), base->getGeometryToObject(),
        base->getObjectToGeometry(), base->getGeometryToWorld(), base->getWorldToGeometry(),
        base->getHasObjectTransform(), base->getHasGeometryTransform(), base->getHasBoundingShapes(),
        base->getHasClippingShapes(), base->getBoundingObjectIndices(), base->getClippingObjectIndices(),
        base->getBakedQuadricStorage(), base->getHasBakedQuadric(), base->getBakedPlaneStorage(),
        base->getHasBakedPlane(), quadricViewpointSlot);
}

CsgProgram *
BakedSceneBuilder::cloneProgramWithReclassification(
    const CsgProgram *base,
    BakedSceneCsgPlanKind planKind,
    bool specializationValid,
    int specializationCoreOperandIndex,
    const java::ArrayList<int> &planeOperandIndices,
    const java::ArrayList<int> &nestedOperandIndices,
    const java::ArrayList<int> &transformedPrimitiveOperandIndices,
    const java::ArrayList<int> &directPrimitiveOperandIndices,
    const java::ArrayList<CsgOperandRecord *> &operands)
{
    return new CsgProgram(
        base->getAlgorithm(), planKind, base->getGeometryType(), base->getTopLevel(),
        specializationValid, specializationCoreOperandIndex,
        planeOperandIndices, nestedOperandIndices, transformedPrimitiveOperandIndices,
        directPrimitiveOperandIndices, operands,
        base->getDirectPrimitiveCullBins(), base->getTransformedPrimitiveCullBins());
}

void
BakedSceneBuilder::compileTracingObjects(
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

TraceableObject *
BakedSceneBuilder::bakeSimpleBody(SimpleBody *object, BakedScene &out)
{
    Geometry *originalGeometry = object->getGeometry();
    Quadric *originalQuadricGeometry = dynamic_cast<Quadric *>(originalGeometry);
    Material *geometryMaterial = object->getGeometryMaterial();
    Material *objectTexture = object->getObjectTexture();
    ColorRgba *objectColor = object->getObjectColor();
    AxisAlignedBoundingBox worldBounds = object->getAABB();
    bool bounded = !worldBounds.isUnbounded();
    bool noShadowFlag = object->getNoShadowFlag();
    bool castsShadow = !noShadowFlag;
    bool hasObjectTransform = object->getTransformation() != nullptr;
    bool hasGeometryTransform = object->getGeometryTransformation() != nullptr;
    bool hasBoundingShapes = object->getBoundingShapes().size() > 0;
    bool hasClippingShapes = object->getClippingShapes().size() > 0;
    Matrix4x4d objectToWorld = copyOrIdentity(object->getTransformation());
    Matrix4x4d worldToObject = copyOrIdentity(object->getTransformationInverse());

    Matrix4x4d geometryToObject = copyOrIdentity(object->getGeometryTransformation());
    Matrix4x4d objectToGeometry = copyOrIdentity(object->getGeometryTransformationInverse());
    Matrix4x4d geometryToWorld = objectToWorld.multiply(geometryToObject);
    Matrix4x4d worldToGeometry = objectToGeometry.multiply(worldToObject);

    Quadric bakedQuadricStorage;
    bool hasBakedQuadric = false;
    InfinitePlane bakedPlaneStorage;
    bool hasBakedPlane = false;

    if (!hasBoundingShapes && !hasClippingShapes &&
        (hasObjectTransform || hasGeometryTransform)) {
        java::ArrayList<TransformStep> combinedSteps(
            object->getGeometrySteps().size() + object->getBodySteps().size());
        for (long int i = 0; i < object->getGeometrySteps().size(); i++) {
            combinedSteps.add(object->getGeometrySteps()[i]);
        }
        for (long int i = 0; i < object->getBodySteps().size(); i++) {
            combinedSteps.add(object->getBodySteps()[i]);
        }
        if (combinedSteps.size() > 0) {
            if (originalQuadricGeometry != nullptr) {
                bakedQuadricStorage =
                    BakedGeometryBaker::bakeQuadric(*originalQuadricGeometry, combinedSteps);
                hasBakedQuadric = true;
                hasObjectTransform = false;
                hasGeometryTransform = false;
                object->setBakedTransformFolded(true);
            } else if (InfinitePlane *plane = dynamic_cast<InfinitePlane *>(originalGeometry)) {
                bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, combinedSteps);
                hasBakedPlane = true;
                hasObjectTransform = false;
                hasGeometryTransform = false;
                object->setBakedTransformFolded(true);
            }
        }
    }

    int csgProgramIndex = -1;
    if (ConstructiveSolidGeometry *csg =
            dynamic_cast<ConstructiveSolidGeometry *>(originalGeometry)) {
        csgProgramIndex = compileConstructiveSolidGeometry(csg, out);
    }

    java::ArrayList<int> boundingObjectIndices;
    java::ArrayList<int> clippingObjectIndices;
    compileTracingObjects(object->getBoundingShapes(), out, boundingObjectIndices);
    compileTracingObjects(object->getClippingShapes(), out, clippingObjectIndices);

    BakedSceneTraceKind kind = classifyTraceableObject(
        originalGeometry != nullptr, hasBoundingShapes || hasClippingShapes, csgProgramIndex >= 0);

    return new TraceableObject(
        kind, object, originalGeometry, originalQuadricGeometry, geometryMaterial, objectTexture,
        objectColor, worldBounds, bounded, castsShadow, noShadowFlag, csgProgramIndex, -1,
        objectToWorld, worldToObject, geometryToObject, objectToGeometry, geometryToWorld,
        worldToGeometry, hasObjectTransform, hasGeometryTransform, hasBoundingShapes,
        hasClippingShapes, boundingObjectIndices, clippingObjectIndices, bakedQuadricStorage,
        hasBakedQuadric, bakedPlaneStorage, hasBakedPlane, -1);
}

bool
BakedSceneBuilder::isPushdownCandidateOperand(const CsgOperandRecord *operand)
{
    return operand->getOriginalQuadricGeometry() != nullptr || operand->getIsInfinitePlane();
}

bool
BakedSceneBuilder::nestedProgramFullyBakeable(const BakedScene &scene, int programIndex, int depthGuard)
{
    if (programIndex < 0 || programIndex >= scene.csgPrograms.size() || depthGuard <= 0) {
        return false;
    }
    const CsgProgram *program = scene.csgPrograms[programIndex];
    for (long int i = 0; i < program->getOperands().size(); i++) {
        const CsgOperandRecord *operand = program->getOperands()[i];
        if (isPushdownCandidateOperand(operand)) {
            continue;
        }
        if (operand->getKind() == BakedSceneCsgOperandKind::NestedCsg &&
            nestedProgramFullyBakeable(scene, operand->getNestedCsgProgramIndex(), depthGuard - 1)) {
            continue;
        }
        return false;
    }
    return true;
}

void
BakedSceneBuilder::pushDownStepsIntoProgram(
    BakedScene &out, int programIndex, const java::ArrayList<TransformStep> &parentSteps,
    const Matrix4x4d &parentForwardTransform)
{
    CsgProgram *program = out.csgPrograms[programIndex];
    java::ArrayList<CsgOperandRecord *> newOperands;
    newOperands.reserve(program->getOperands().size());

    for (long int i = 0; i < program->getOperands().size(); i++) {
        CsgOperandRecord *operand = program->getOperands()[i];

        AxisAlignedBoundingBox bakedBounds = operand->getBakedBounds();
        bool bounded = operand->getBounded();
        if (!bakedBounds.isUnbounded()) {
            bakedBounds = AxisAlignedBoundingBox::fromTransformedCorners(
                bakedBounds.min, bakedBounds.max, &parentForwardTransform);
            bounded = !bakedBounds.isUnbounded();
        }

        if (operand->getKind() == BakedSceneCsgOperandKind::NestedCsg) {
            pushDownStepsIntoProgram(
                out, operand->getNestedCsgProgramIndex(), parentSteps, parentForwardTransform);
            // Whether this inner wrapper was itself folded earlier (matrices
            // already identity) or was never transformed (identity by
            // construction), its program is now world-space - mark it so
            // buildCsgExecutionPlan keeps it on the compiled kernel.
            newOperands.add(cloneOperandWithPushdownBake(
                operand, bakedBounds, bounded, operand->getKind(), operand->getHasTransform(),
                operand->getObjectToLocal(), operand->getLocalToObject(), true,
                operand->getBakedQuadricStorage(), operand->getHasBakedQuadric(),
                operand->getBakedPlaneStorage(), operand->getHasBakedPlane(),
                operand->getPlaneNormal(), operand->getPlaneDistance(),
                operand->getPushdownAccumulatedSteps()));
            continue;
        }
        if (operand->getOriginalQuadricGeometry() == nullptr && !operand->getIsInfinitePlane()) {
            // nestedProgramFullyBakeable should have ruled this out already;
            // leave untouched rather than mis-collapse an unbakeable kind.
            newOperands.add(cloneOperandWithPushdownBake(
                operand, bakedBounds, bounded, operand->getKind(), operand->getHasTransform(),
                operand->getObjectToLocal(), operand->getLocalToObject(), operand->getPushdownFolded(),
                operand->getBakedQuadricStorage(), operand->getHasBakedQuadric(),
                operand->getBakedPlaneStorage(), operand->getHasBakedPlane(),
                operand->getPlaneNormal(), operand->getPlaneDistance(),
                operand->getPushdownAccumulatedSteps()));
            continue;
        }

        const java::ArrayList<TransformStep> &baseSteps =
            (operand->getHasBakedQuadric() || operand->getHasBakedPlane()) ?
                operand->getPushdownAccumulatedSteps() :
                (operand->getOperand() != nullptr ?
                    operand->getOperand()->getSteps() : operand->getPushdownAccumulatedSteps());
        java::ArrayList<TransformStep> combinedSteps(baseSteps.size() + parentSteps.size());
        for (long int s = 0; s < baseSteps.size(); s++) {
            combinedSteps.add(baseSteps[s]);
        }
        for (long int s = 0; s < parentSteps.size(); s++) {
            combinedSteps.add(parentSteps[s]);
        }

        Quadric bakedQuadricStorage = operand->getBakedQuadricStorage();
        bool hasBakedQuadric = operand->getHasBakedQuadric();
        InfinitePlane bakedPlaneStorage = operand->getBakedPlaneStorage();
        bool hasBakedPlane = operand->getHasBakedPlane();
        Vector3Dd planeNormal = operand->getPlaneNormal();
        double planeDistance = operand->getPlaneDistance();

        if (operand->getOriginalQuadricGeometry() != nullptr) {
            bakedQuadricStorage = BakedGeometryBaker::bakeQuadric(
                *operand->getOriginalQuadricGeometry(), combinedSteps);
            hasBakedQuadric = true;
        } else {
            InfinitePlane *plane = static_cast<InfinitePlane *>(operand->getOriginalGeometry());
            bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, combinedSteps);
            hasBakedPlane = true;
            planeNormal = bakedPlaneStorage.getNormalVector();
            planeDistance = bakedPlaneStorage.getDistance();
        }
        if (operand->getOperand() != nullptr) {
            operand->getOperand()->setBakedTransformFolded(true);
        }

        Geometry *effectiveGeometry =
            hasBakedQuadric ? static_cast<Geometry *>(&bakedQuadricStorage) :
                (hasBakedPlane ? static_cast<Geometry *>(&bakedPlaneStorage) : operand->getOriginalGeometry());
        Quadric *effectiveQuadricGeometry =
            hasBakedQuadric ? &bakedQuadricStorage : operand->getOriginalQuadricGeometry();
        BakedSceneCsgOperandKind newKind = classifyOperandKind(
            effectiveGeometry, operand->getNestedCsgProgramIndex(), false,
            operand->getIsInfinitePlane(), effectiveQuadricGeometry);

        newOperands.add(cloneOperandWithPushdownBake(
            operand, bakedBounds, bounded, newKind, false,
            operand->getObjectToLocal(), operand->getLocalToObject(), operand->getPushdownFolded(),
            bakedQuadricStorage, hasBakedQuadric, bakedPlaneStorage, hasBakedPlane,
            planeNormal, planeDistance, combinedSteps));
    }

    // Re-classify from scratch: classifyCsgProgramSpecialization only ever
    // SETS specializationValid/planKind, it never clears them, so a
    // specialization whose precondition no longer holds after the bounds
    // moved into parent space (e.g. DisjointBoundedUnion boxes that now
    // overlap after corner-transformation) would silently stay latched.
    BakedSceneCsgPlanKind planKind = BakedSceneCsgPlanKind::Fallback;
    bool specializationValid = false;
    int specializationCoreOperandIndex = -1;
    classifyCsgProgramSpecialization(
        program->getGeometryType(), program->getTopLevel(), newOperands,
        planKind, specializationValid, specializationCoreOperandIndex);

    java::ArrayList<int> planeOperandIndices;
    java::ArrayList<int> nestedOperandIndices;
    java::ArrayList<int> transformedPrimitiveOperandIndices;
    java::ArrayList<int> directPrimitiveOperandIndices;
    buildCsgExecutionPlan(
        program->getAlgorithm(), specializationValid, planKind, newOperands, out,
        planeOperandIndices, nestedOperandIndices, transformedPrimitiveOperandIndices,
        directPrimitiveOperandIndices);

    CsgProgram *replacement = cloneProgramWithReclassification(
        program, planKind, specializationValid, specializationCoreOperandIndex,
        planeOperandIndices, nestedOperandIndices, transformedPrimitiveOperandIndices,
        directPrimitiveOperandIndices, newOperands);
    out.csgPrograms[programIndex] = replacement;
    // `program`'s own (now orphaned) operand pointers are freed by its own
    // destructor; `newOperands` are all fresh clones, never aliasing them.
    delete program;
}

CsgOperandRecord *
BakedSceneBuilder::bakeCsgOperand(CsgOperand *operand, BakedScene &out)
{
    Geometry *originalGeometry = operand->getGeometry();
    Material *material = operand->getMaterial();
    bool hasTransform = operand->getTransformation() != nullptr;
    Matrix4x4d objectToLocal = copyOrIdentity(operand->getTransformation());
    Matrix4x4d localToObject = copyOrIdentity(operand->getTransformationInverse());
    AxisAlignedBoundingBox bakedBounds = operand->getBakedBounds();
    bool bounded = operand->hasBoundedBakedBounds();
    bool isInfinitePlane = dynamic_cast<InfinitePlane *>(originalGeometry) != nullptr;
    Quadric *originalQuadricGeometry = dynamic_cast<Quadric *>(originalGeometry);
    Vector3Dd planeNormal(0.0, 1.0, 0.0);
    double planeDistance = 0.0;
    if (InfinitePlane *plane = dynamic_cast<InfinitePlane *>(originalGeometry)) {
        planeNormal = plane->getNormalVector();
        planeDistance = plane->getDistance();
    }

    Quadric bakedQuadricStorage;
    bool hasBakedQuadric = false;
    InfinitePlane bakedPlaneStorage;
    bool hasBakedPlane = false;
    java::ArrayList<TransformStep> pushdownAccumulatedSteps;

    constexpr bool kEnableCoefficientCollapse = true;
    if (kEnableCoefficientCollapse &&
        hasTransform && operand->getSteps().size() > 0) {
        if (originalQuadricGeometry != nullptr) {
            bakedQuadricStorage =
                BakedGeometryBaker::bakeQuadric(*originalQuadricGeometry, operand->getSteps());
            hasBakedQuadric = true;
            hasTransform = false;
            pushdownAccumulatedSteps = operand->getSteps();
            operand->setBakedTransformFolded(true);
        } else if (isInfinitePlane) {
            InfinitePlane *plane = static_cast<InfinitePlane *>(originalGeometry);
            bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, operand->getSteps());
            hasBakedPlane = true;
            hasTransform = false;
            planeNormal = bakedPlaneStorage.getNormalVector();
            planeDistance = bakedPlaneStorage.getDistance();
            pushdownAccumulatedSteps = operand->getSteps();
            operand->setBakedTransformFolded(true);
        }
    }

    bool cullSafe =
        bounded &&
        (dynamic_cast<Sphere *>(originalGeometry) != nullptr ||
         dynamic_cast<Box *>(originalGeometry) != nullptr ||
         dynamic_cast<Blob *>(originalGeometry) != nullptr ||
         dynamic_cast<Quadric *>(originalGeometry) != nullptr ||
         dynamic_cast<HeightField *>(originalGeometry) != nullptr);

    int nestedCsgProgramIndex = -1;
    bool pushdownFolded = false;
    if (ConstructiveSolidGeometry *nestedCsg =
            dynamic_cast<ConstructiveSolidGeometry *>(originalGeometry)) {
        nestedCsgProgramIndex = compileConstructiveSolidGeometry(nestedCsg, out);
        cullSafe = false;
        isInfinitePlane = false;

        constexpr bool kEnableNestedTransformPushdown = true;
        constexpr int kPushdownDepthGuard = 16;
        if (kEnableNestedTransformPushdown &&
            hasTransform && operand->getSteps().size() > 0 &&
            nestedProgramFullyBakeable(out, nestedCsgProgramIndex, kPushdownDepthGuard)) {
            pushDownStepsIntoProgram(
                out, nestedCsgProgramIndex, operand->getSteps(), objectToLocal);
            hasTransform = false;
            objectToLocal = Matrix4x4d::identityMatrix();
            localToObject = Matrix4x4d::identityMatrix();
            pushdownFolded = true;
            operand->setBakedTransformFolded(true);
        }
    }

    Geometry *effectiveGeometry =
        hasBakedQuadric ? static_cast<Geometry *>(&bakedQuadricStorage) :
            (hasBakedPlane ? static_cast<Geometry *>(&bakedPlaneStorage) : originalGeometry);
    Quadric *effectiveQuadricGeometry = hasBakedQuadric ? &bakedQuadricStorage : originalQuadricGeometry;
    BakedSceneCsgOperandKind kind = classifyOperandKind(
        effectiveGeometry, nestedCsgProgramIndex, hasTransform, isInfinitePlane, effectiveQuadricGeometry);

    return new CsgOperandRecord(
        kind, operand, originalGeometry, originalQuadricGeometry, material, nestedCsgProgramIndex,
        bakedBounds, objectToLocal, localToObject, hasTransform, bounded, cullSafe, isInfinitePlane,
        planeNormal, planeDistance, -1, -1, bakedQuadricStorage, hasBakedQuadric, bakedPlaneStorage,
        hasBakedPlane, false, -1, false, false,
        java::ArrayList<int>(), java::ArrayList<int>(), pushdownAccumulatedSteps, pushdownFolded);
}

void
BakedSceneBuilder::sortCullSafeEntriesByKey(java::ArrayList<CullSafeEntry> &entries)
{
    for (long int i = 1; i < entries.size(); i++) {
        const CullSafeEntry pivot = entries[i];
        long int j = i - 1;
        while (j >= 0 && entries[j].key > pivot.key) {
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = pivot;
    }
}

OperandCullBins *
BakedSceneBuilder::buildOperandCullBinsForBucket(
    const java::ArrayList<int> &bucket,
    const java::ArrayList<CsgOperandRecord *> &operands)
{
    const long int bucketSize = bucket.size();
    if (!kEnableOperandCullBins || bucketSize < kOperandCullBinThreshold) {
        return nullptr;
    }

    java::ArrayList<int> alwaysTestedPositions;
    java::ArrayList<CullSafeEntry> cullSafeByAxis{bucketSize};
    Vector3Dd lo(1e30, 1e30, 1e30);
    Vector3Dd hi(-1e30, -1e30, -1e30);
    for (long int p = 0; p < bucketSize; p++) {
        const CsgOperandRecord *operand = operands[bucket[p]];
        if (operand->getBounded() && operand->getCullSafe()) {
            const Vector3Dd c = operand->getBakedBounds().centroid();
            cullSafeByAxis.add(CullSafeEntry{0.0, (int)p});
            lo = Vector3Dd(
                std::fmin(lo.x(), c.x()), std::fmin(lo.y(), c.y()), std::fmin(lo.z(), c.z()));
            hi = Vector3Dd(
                std::fmax(hi.x(), c.x()), std::fmax(hi.y(), c.y()), std::fmax(hi.z(), c.z()));
        } else {
            alwaysTestedPositions.add((int)p);
        }
    }

    if (cullSafeByAxis.size() < kOperandCullBinThreshold) {
        return nullptr;
    }

    const double spreadX = hi.x() - lo.x();
    const double spreadY = hi.y() - lo.y();
    const double spreadZ = hi.z() - lo.z();
    int axis = 0;
    if (spreadY > spreadX && spreadY >= spreadZ) {
        axis = 1;
    } else if (spreadZ > spreadX && spreadZ > spreadY) {
        axis = 2;
    }

    for (long int i = 0; i < cullSafeByAxis.size(); i++) {
        CullSafeEntry &entry = cullSafeByAxis[i];
        const Vector3Dd c = operands[bucket[entry.position]]->getBakedBounds().centroid();
        entry.key = axis == 0 ? c.x() : (axis == 1 ? c.y() : c.z());
    }
    sortCullSafeEntriesByKey(cullSafeByAxis);

    const int total = (int)cullSafeByAxis.size();
    int numBins = (int)std::sqrt((double)total);
    if (numBins < 1) {
        numBins = 1;
    }
    if (numBins > 64) {
        numBins = 64;
    }
    const int base = total / numBins;
    const int remainder = total % numBins;

    java::ArrayList<AxisAlignedBoundingBox> binBounds;
    java::ArrayList<int> binMemberStart;
    java::ArrayList<int> binMemberCount;
    java::ArrayList<int> binMembers;
    binMembers.reserve(total);
    int cursor = 0;
    for (int b = 0; b < numBins; b++) {
        const int count = base + (b < remainder ? 1 : 0);
        AxisAlignedBoundingBox aggregate = AxisAlignedBoundingBox::empty();
        const int start = (int)binMembers.size();
        for (int k = 0; k < count; k++) {
            const int pos = cullSafeByAxis[cursor].position;
            binMembers.add(pos);
            aggregate = aggregate.enclosing(operands[bucket[pos]]->getBakedBounds());
            cursor++;
        }
        binBounds.add(aggregate);
        binMemberStart.add(start);
        binMemberCount.add(count);
    }
    return new OperandCullBins(
        true, binBounds, binMemberStart, binMemberCount, binMembers, alwaysTestedPositions);
}

CsgProgram *
BakedSceneBuilder::bakeConstructiveSolidGeometry(ConstructiveSolidGeometry *geometry, BakedScene &out)
{
    BooleanSetOperations geometryType = geometry->getGeometryType();
    BakedSceneCsgAlgorithm algorithm;
    bool topLevel;
    if (ConstructiveSolidGeometryByRaySegment *raySegment =
            dynamic_cast<ConstructiveSolidGeometryByRaySegment *>(geometry)) {
        algorithm = BakedSceneCsgAlgorithm::RaySegments;
        topLevel = raySegment->isTopLevel();
    } else {
        algorithm = BakedSceneCsgAlgorithm::MorganRules;
        topLevel = false;
    }

    const java::ArrayList<CsgOperand*> &sourceOperands = geometry->getOperands();
    java::ArrayList<CsgOperandRecord *> operands;
    operands.reserve(sourceOperands.size());
    for (long int i = 0; i < sourceOperands.size(); i++) {
        if (sourceOperands[i] == nullptr) {
            continue;
        }
        operands.add(bakeCsgOperand(sourceOperands[i], out));
    }

    BakedSceneCsgPlanKind planKind = BakedSceneCsgPlanKind::Fallback;
    bool specializationValid = false;
    int specializationCoreOperandIndex = -1;
    classifyCsgProgramSpecialization(
        geometryType, topLevel, operands, planKind, specializationValid, specializationCoreOperandIndex);

    java::ArrayList<int> planeOperandIndices;
    java::ArrayList<int> nestedOperandIndices;
    java::ArrayList<int> transformedPrimitiveOperandIndices;
    java::ArrayList<int> directPrimitiveOperandIndices;
    buildCsgExecutionPlan(
        algorithm, specializationValid, planKind, operands, out,
        planeOperandIndices, nestedOperandIndices, transformedPrimitiveOperandIndices,
        directPrimitiveOperandIndices);

    const OperandCullBins *directPrimitiveCullBins = nullptr;
    const OperandCullBins *transformedPrimitiveCullBins = nullptr;
    if (geometryType == BooleanSetOperations::UNION) {
        OperandCullBins *directBins = buildOperandCullBinsForBucket(directPrimitiveOperandIndices, operands);
        if (directBins != nullptr) {
            out.operandCullBinsStorage.add(directBins);
            directPrimitiveCullBins = directBins;
        }
        OperandCullBins *transformedBins =
            buildOperandCullBinsForBucket(transformedPrimitiveOperandIndices, operands);
        if (transformedBins != nullptr) {
            out.operandCullBinsStorage.add(transformedBins);
            transformedPrimitiveCullBins = transformedBins;
        }
    }
    return new CsgProgram(
        algorithm, planKind, geometryType, topLevel, specializationValid, specializationCoreOperandIndex,
        planeOperandIndices, nestedOperandIndices, transformedPrimitiveOperandIndices,
        directPrimitiveOperandIndices, operands, directPrimitiveCullBins, transformedPrimitiveCullBins);
}

int
BakedSceneBuilder::compileConstructiveSolidGeometry(ConstructiveSolidGeometry *geometry, BakedScene &out)
{
    const int index = out.csgPrograms.size();
    out.csgPrograms.add(nullptr);
    out.csgPrograms.set(index, bakeConstructiveSolidGeometry(geometry, out));
    return index;
}

CompositeRecord *
BakedSceneBuilder::bakeComposite(Composite *object, BakedScene &out)
{
    AxisAlignedBoundingBox worldBounds = object->getAABB();
    bool bounded = !worldBounds.isUnbounded();
    bool noShadowFlag = object->getNoShadowFlag();
    bool hasObjectTransform = object->getTransformation() != nullptr;
    bool hasBoundingShapes = object->getBoundingShapes().size() > 0;
    bool hasClippingShapes = object->getClippingShapes().size() > 0;
    Matrix4x4d objectToWorld = copyOrIdentity(object->getTransformation());
    Matrix4x4d worldToObject = copyOrIdentity(object->getTransformationInverse());

    java::ArrayList<int> boundingObjectIndices;
    java::ArrayList<int> clippingObjectIndices;
    compileTracingObjects(object->getBoundingShapes(), out, boundingObjectIndices);
    compileTracingObjects(object->getClippingShapes(), out, clippingObjectIndices);

    const java::ArrayList<SimpleBody*> &children = object->getSimpleBodies();
    java::ArrayList<int> childObjectIndices;
    childObjectIndices.reserve(children.size());
    for (long int i = 0; i < children.size(); i++) {
        childObjectIndices.add(compileTracingObject(children[i], out));
    }

    return new CompositeRecord(
        object, worldBounds, objectToWorld, worldToObject, bounded, noShadowFlag, hasObjectTransform,
        hasBoundingShapes, hasClippingShapes, boundingObjectIndices, clippingObjectIndices,
        childObjectIndices);
}

int
BakedSceneBuilder::compileTracingObject(SimpleBody *object, BakedScene &out)
{
    const int index = out.traceableObjects.size();
    out.traceableObjects.add(nullptr);

    if (isBakeableSimpleBody(object)) {
        TraceableObject *baked = bakeSimpleBody(object, out);
        out.traceableObjects.set(index, baked);
        return index;
    }

    Composite *composite = dynamic_cast<Composite *>(object);
    if (composite != nullptr) {
        const int compositeIndex = out.composites.size();
        // Reserve the parent slot before compiling children: nested
        // composites may append their own baked nodes during bakeComposite().
        out.composites.add(nullptr);
        CompositeRecord *bakedComposite = bakeComposite(composite, out);
        out.composites.set(compositeIndex, bakedComposite);

        TraceableObject *entry = new TraceableObject(
            BakedSceneTraceKind::Composite, composite, nullptr, nullptr, nullptr, nullptr, nullptr,
            bakedComposite->getWorldBounds(), bakedComposite->getBounded(),
            !bakedComposite->getNoShadowFlag(), bakedComposite->getNoShadowFlag(), -1, compositeIndex,
            bakedComposite->getObjectToWorld(), bakedComposite->getWorldToObject(),
            Matrix4x4d::identityMatrix(), Matrix4x4d::identityMatrix(),
            Matrix4x4d::identityMatrix(), Matrix4x4d::identityMatrix(),
            bakedComposite->getHasObjectTransform(), false,
            bakedComposite->getHasBoundingShapes(), bakedComposite->getHasClippingShapes(),
            java::ArrayList<int>(), java::ArrayList<int>(),
            Quadric(), false, InfinitePlane(), false, -1);
        out.traceableObjects.set(index, entry);
        return index;
    }

    return index;
}

void
BakedSceneBuilder::accumulateStatistics(BakedScene &scene, long quadricViewpointSlotCount, long planeViewpointSlotCount)
{
    long countByKind[6] = {0, 0, 0, 0, 0, 0};
    for (long int i = 0; i < scene.traceableObjects.size(); i++) {
        const int kindIndex = (int)scene.traceableObjects[i]->getKind();
        if (kindIndex >= 0 && kindIndex < 6) {
            countByKind[kindIndex]++;
        }
    }

    const long topLevelObjectCount = scene.topLevelObjectIndices.size();
    long topLevelObjectCullSafeCount = 0;
    for (long int i = 0; i < scene.topLevelObjectIndices.size(); i++) {
        const int objectIndex = scene.topLevelObjectIndices[i];
        const TraceableObject *object = scene.traceableObjects[objectIndex];
        if (object->getBounded()) {
            topLevelObjectCullSafeCount++;
        }
    }

    const long csgProgramCount = scene.csgPrograms.size();
    long csgPlanTopLevelPlaneUnion = 0;
    long csgPlanDisjointBoundedUnion = 0;
    long csgPlanSingleCorePlaneIntersection = 0;
    long csgPlanGenericMorgan = 0;
    long csgPlanGenericRaySegments = 0;
    long csgPlanFallback = 0;
    long residualBakedQuadricOperands = 0;
    long residualBakedPlaneOperands = 0;
    long residualTransformedOperands = 0;
    long residualCategory1NestedCsg = 0;
    long residualCategory1PushdownEligible = 0;
    long residualCategory2EmptySteps = 0;
    long residualCategory3Unbakeable = 0;
    long unionProgramOperandHistogram[4] = {0, 0, 0, 0};
    long unionProgramOperandCullSafeCount = 0;
    long unionProgramOperandTotalCount = 0;

    for (long int i = 0; i < scene.csgPrograms.size(); i++) {
        const CsgProgram *program = scene.csgPrograms[i];
        switch (program->getPlanKind()) {
        case BakedSceneCsgPlanKind::TopLevelPlaneUnion:
            csgPlanTopLevelPlaneUnion++;
            break;
        case BakedSceneCsgPlanKind::DisjointBoundedUnion:
            csgPlanDisjointBoundedUnion++;
            break;
        case BakedSceneCsgPlanKind::SingleCorePlaneIntersection:
            csgPlanSingleCorePlaneIntersection++;
            break;
        case BakedSceneCsgPlanKind::GenericMorgan:
            csgPlanGenericMorgan++;
            break;
        case BakedSceneCsgPlanKind::GenericRaySegments:
            csgPlanGenericRaySegments++;
            break;
        case BakedSceneCsgPlanKind::Fallback:
            csgPlanFallback++;
            break;
        }
        if (program->getGeometryType() == BooleanSetOperations::UNION) {
            const long int operandCount = program->getOperands().size();
            int bucket;
            if (operandCount <= 4) {
                bucket = 0;
            } else if (operandCount <= 16) {
                bucket = 1;
            } else if (operandCount <= 64) {
                bucket = 2;
            } else {
                bucket = 3;
            }
            unionProgramOperandHistogram[bucket]++;
        }
        for (long int j = 0; j < program->getOperands().size(); j++) {
            const CsgOperandRecord *operand = program->getOperands()[j];
            if (program->getGeometryType() == BooleanSetOperations::UNION) {
                unionProgramOperandTotalCount++;
                if (operand->getBounded() && operand->getCullSafe()) {
                    unionProgramOperandCullSafeCount++;
                }
            }
            if (operand->getHasBakedQuadric()) {
                residualBakedQuadricOperands++;
            }
            if (operand->getHasBakedPlane()) {
                residualBakedPlaneOperands++;
            }
            if (operand->getHasTransform()) {
                residualTransformedOperands++;

                switch (operand->getKind()) {
                case BakedSceneCsgOperandKind::TransformedNestedCsg:
                    residualCategory1NestedCsg++;
                    if (nestedProgramFullyBakeable(scene, operand->getNestedCsgProgramIndex(), 16)) {
                        residualCategory1PushdownEligible++;
                    }
                    break;
                case BakedSceneCsgOperandKind::TransformedQuadric:
                case BakedSceneCsgOperandKind::TransformedPlane:
                    if (operand->getOperand() != nullptr &&
                        operand->getOperand()->getSteps().size() == 0) {
                        residualCategory2EmptySteps++;
                    }
                    break;
                case BakedSceneCsgOperandKind::TransformedSphere:
                case BakedSceneCsgOperandKind::TransformedPrimitive:
                    residualCategory3Unbakeable++;
                    break;
                default:
                    break;
                }
            }
        }
    }

    delete scene.statistics;
    scene.statistics = new BakedSceneStatistics(
        countByKind, csgProgramCount, csgPlanTopLevelPlaneUnion, csgPlanDisjointBoundedUnion,
        csgPlanSingleCorePlaneIntersection, csgPlanGenericMorgan, csgPlanGenericRaySegments,
        csgPlanFallback, residualBakedQuadricOperands, residualBakedPlaneOperands,
        residualTransformedOperands, quadricViewpointSlotCount, planeViewpointSlotCount,
        residualCategory1NestedCsg, residualCategory1PushdownEligible, residualCategory2EmptySteps,
        residualCategory3Unbakeable, unionProgramOperandHistogram, unionProgramOperandCullSafeCount,
        unionProgramOperandTotalCount, topLevelObjectCount, topLevelObjectCullSafeCount);
}

void
BakedSceneBuilder::build(const java::ArrayList<SimpleBody*> &objects, BakedScene &out)
{
    out.clearOwnedContents();
    delete out.statistics;
    out.statistics = nullptr;
    out.topLevelObjectIndices.clear();
    out.boundedObjectIndices.clear();
    out.unboundedObjectIndices.clear();
    out.shadowCastingObjectIndices.clear();
    out.boundedShadowCastingObjectIndices.clear();
    out.unboundedShadowCastingObjectIndices.clear();

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

        const TraceableObject *entry = out.traceableObjects[index];
        if (entry->getBounded()) {
            out.boundedObjectIndices.add(index);
        } else {
            out.unboundedObjectIndices.add(index);
        }
        if (entry->getCastsShadow()) {
            out.shadowCastingObjectIndices.add(index);
            if (entry->getBounded()) {
                out.boundedShadowCastingObjectIndices.add(index);
            } else {
                out.unboundedShadowCastingObjectIndices.add(index);
            }
        }
    }

    // Sequential viewpoint-slot assignment across every CsgProgram's
    // operands (program order, then operand order) and then every
    // traceableObject - same global order as before. Each record is
    // immutable, so "assigning" a slot means constructing a fresh record
    // with the slot filled in and replacing the owning pointer.
    int nextQuadricSlot = 0;
    int nextPlaneSlot = 0;
    for (long int i = 0; i < out.csgPrograms.size(); i++) {
        CsgProgram *program = out.csgPrograms[i];
        java::ArrayList<CsgOperandRecord *> updatedOperands;
        updatedOperands.reserve(program->getOperands().size());
        for (long int j = 0; j < program->getOperands().size(); j++) {
            CsgOperandRecord *operand = program->getOperands()[j];
            int quadricSlot = operand->getQuadricViewpointSlot();
            int planeSlot = operand->getPlaneViewpointSlot();
            if (operand->getQuadricGeometry() != nullptr) {
                quadricSlot = nextQuadricSlot++;
            }
            if (operand->getKind() == BakedSceneCsgOperandKind::DirectPlane ||
                operand->getKind() == BakedSceneCsgOperandKind::TransformedPlane) {
                planeSlot = nextPlaneSlot++;
            }
            updatedOperands.add(cloneOperandWithViewpointSlots(operand, quadricSlot, planeSlot));
        }
        CsgProgram *replacement = cloneProgramWithReclassification(
            program, program->getPlanKind(), program->getSpecializationValid(),
            program->getSpecializationCoreOperandIndex(), program->getPlaneOperandIndices(),
            program->getNestedOperandIndices(), program->getTransformedPrimitiveOperandIndices(),
            program->getDirectPrimitiveOperandIndices(), updatedOperands);
        out.csgPrograms[i] = replacement;
        delete program;
    }
    for (long int i = 0; i < out.traceableObjects.size(); i++) {
        TraceableObject *object = out.traceableObjects[i];
        if (object->getQuadricGeometry() != nullptr) {
            TraceableObject *replacement = cloneObjectWithViewpointSlot(object, nextQuadricSlot++);
            out.traceableObjects[i] = replacement;
            delete object;
        }
    }

    accumulateStatistics(out, nextQuadricSlot, nextPlaneSlot);
}
