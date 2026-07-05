

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

BakedScene::TraceKind
BakedSceneBuilder::classifyTraceableObject(bool hasGeometry, bool boundedOrClipped, bool hasCsg)
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
BakedSceneBuilder::classifyOperandKind(const BakedScene::CsgOperandRecord &baked)
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
BakedSceneBuilder::hasFiniteInteriorBounds(const BakedScene::CsgOperandRecord &operand)
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
BakedSceneBuilder::areSeparated(const AxisAlignedBoundingBox &left, const AxisAlignedBoundingBox &right)
{
    return
        left.max.x() < right.min.x() || right.max.x() < left.min.x() ||
        left.max.y() < right.min.y() || right.max.y() < left.min.y() ||
        left.max.z() < right.min.z() || right.max.z() < left.min.z();
}

bool
BakedSceneBuilder::hasPairwiseDisjointFiniteOperands(
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

void
BakedSceneBuilder::classifyCsgProgramSpecialization(BakedScene::CsgProgram &baked)
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
BakedSceneBuilder::buildCsgExecutionPlan(BakedScene::CsgProgram &baked, const BakedScene &out)
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
            if ((operand.kind == BakedScene::CsgOperandKind::TransformedNestedCsg ||
                 (operand.kind == BakedScene::CsgOperandKind::NestedCsg &&
                  operand.pushdownFolded)) &&
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

BakedScene::TraceableObject
BakedSceneBuilder::bakeSimpleBody(SimpleBody *object, BakedScene &out)
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

bool
BakedSceneBuilder::isPushdownCandidateOperand(const BakedScene::CsgOperandRecord &operand)
{
    return operand.quadricGeometry != nullptr || operand.isInfinitePlane;
}

bool
BakedSceneBuilder::nestedProgramFullyBakeable(const BakedScene &scene, int programIndex, int depthGuard)
{
    if (programIndex < 0 || programIndex >= scene.csgPrograms.size() || depthGuard <= 0) {
        return false;
    }
    const BakedScene::CsgProgram &program = scene.csgPrograms[programIndex];
    for (long int i = 0; i < program.operands.size(); i++) {
        const BakedScene::CsgOperandRecord &operand = program.operands[i];
        if (isPushdownCandidateOperand(operand)) {
            continue;
        }
        if (operand.kind == BakedScene::CsgOperandKind::NestedCsg &&
            nestedProgramFullyBakeable(scene, operand.nestedCsgProgramIndex, depthGuard - 1)) {
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
    BakedScene::CsgProgram &program = out.csgPrograms[programIndex];
    for (long int i = 0; i < program.operands.size(); i++) {
        BakedScene::CsgOperandRecord &operand = program.operands[i];
        if (!operand.bakedBounds.isUnbounded()) {
            operand.bakedBounds = AxisAlignedBoundingBox::fromTransformedCorners(
                operand.bakedBounds.min, operand.bakedBounds.max, &parentForwardTransform);
            operand.bounded = !operand.bakedBounds.isUnbounded();
        }
        if (operand.kind == BakedScene::CsgOperandKind::NestedCsg) {
            pushDownStepsIntoProgram(
                out, operand.nestedCsgProgramIndex, parentSteps, parentForwardTransform);
            // Whether this inner wrapper was itself folded earlier (matrices
            // already identity) or was never transformed (identity by
            // construction), its program is now world-space - mark it so
            // buildCsgExecutionPlan keeps it on the compiled kernel.
            operand.pushdownFolded = true;
            continue;
        }
        if (operand.quadricGeometry == nullptr && !operand.isInfinitePlane) {
            // nestedProgramFullyBakeable should have ruled this out already;
            // leave untouched rather than mis-collapse an unbakeable kind.
            continue;
        }

        const java::ArrayList<TransformStep> &baseSteps =
            (operand.hasBakedQuadric || operand.hasBakedPlane) ?
                operand.pushdownAccumulatedSteps :
                (operand.operand != nullptr ?
                    operand.operand->getSteps() : operand.pushdownAccumulatedSteps);
        java::ArrayList<TransformStep> combinedSteps(baseSteps.size() + parentSteps.size());
        for (long int s = 0; s < baseSteps.size(); s++) {
            combinedSteps.add(baseSteps[s]);
        }
        for (long int s = 0; s < parentSteps.size(); s++) {
            combinedSteps.add(parentSteps[s]);
        }

        if (operand.quadricGeometry != nullptr) {
            operand.bakedQuadricStorage =
                BakedGeometryBaker::bakeQuadric(*operand.quadricGeometry, combinedSteps);
            operand.hasBakedQuadric = true;
        } else {
            InfinitePlane *plane = static_cast<InfinitePlane *>(operand.geometry);
            operand.bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, combinedSteps);
            operand.hasBakedPlane = true;
            operand.planeNormal = operand.bakedPlaneStorage.getNormalVector();
            operand.planeDistance = operand.bakedPlaneStorage.getDistance();
        }
        operand.pushdownAccumulatedSteps = combinedSteps;
        operand.hasTransform = false;
        if (operand.operand != nullptr) {
            operand.operand->setBakedTransformFolded(true);
        }
        operand.kind = classifyOperandKind(operand);
    }
    // Re-classify from scratch: classifyCsgProgramSpecialization only ever
    // SETS specializationValid/planKind, it never clears them, so a
    // specialization whose precondition no longer holds after the bounds
    // moved into parent space (e.g. DisjointBoundedUnion boxes that now
    // overlap after corner-transformation) would silently stay latched.
    program.specializationValid = false;
    program.specializationCoreOperandIndex = -1;
    classifyCsgProgramSpecialization(program);
    buildCsgExecutionPlan(program, out);
}

BakedScene::CsgOperandRecord
BakedSceneBuilder::bakeCsgOperand(CsgOperand *operand, BakedScene &out)
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

    constexpr bool kEnableCoefficientCollapse = true;
    if (kEnableCoefficientCollapse &&
        baked.hasTransform && operand->getSteps().size() > 0) {
        if (baked.quadricGeometry != nullptr) {
            baked.bakedQuadricStorage =
                BakedGeometryBaker::bakeQuadric(*baked.quadricGeometry, operand->getSteps());
            baked.hasBakedQuadric = true;
            baked.hasTransform = false;
            baked.pushdownAccumulatedSteps = operand->getSteps();
            operand->setBakedTransformFolded(true);
        } else if (baked.isInfinitePlane) {
            InfinitePlane *plane = static_cast<InfinitePlane *>(baked.geometry);
            baked.bakedPlaneStorage = BakedGeometryBaker::bakePlane(*plane, operand->getSteps());
            baked.hasBakedPlane = true;
            baked.hasTransform = false;
            baked.planeNormal = baked.bakedPlaneStorage.getNormalVector();
            baked.planeDistance = baked.bakedPlaneStorage.getDistance();
            baked.pushdownAccumulatedSteps = operand->getSteps();
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

        constexpr bool kEnableNestedTransformPushdown = true;
        constexpr int kPushdownDepthGuard = 16;
        if (kEnableNestedTransformPushdown &&
            baked.hasTransform && operand->getSteps().size() > 0 &&
            nestedProgramFullyBakeable(out, baked.nestedCsgProgramIndex, kPushdownDepthGuard)) {
            pushDownStepsIntoProgram(
                out, baked.nestedCsgProgramIndex, operand->getSteps(), baked.objectToLocal);
            baked.hasTransform = false;
            baked.objectToLocal = Matrix4x4d::identityMatrix();
            baked.localToObject = Matrix4x4d::identityMatrix();
            baked.pushdownFolded = true;
            operand->setBakedTransformFolded(true);
        }
    }
    baked.kind = classifyOperandKind(baked);
    return baked;
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

// Bake-time only. Stores bucket *positions* (see BakedScene::OperandCullBins),
// never operand pointers or global indices, so nothing dangles across later
// ArrayList relocations.
void
BakedSceneBuilder::buildOperandCullBinsForBucket(
    const java::ArrayList<int> &bucket,
    const java::ArrayList<BakedScene::CsgOperandRecord> &operands,
    BakedScene::OperandCullBins &out)
{
    out.built = false;
    out.binBounds.clear();
    out.binMemberStart.clear();
    out.binMemberCount.clear();
    out.binMembers.clear();
    out.alwaysTestedPositions.clear();

    const long int bucketSize = bucket.size();
    if (!kEnableOperandCullBins || bucketSize < kOperandCullBinThreshold) {
        return;
    }

    java::ArrayList<CullSafeEntry> cullSafeByAxis{bucketSize};
    Vector3Dd lo(1e30, 1e30, 1e30);
    Vector3Dd hi(-1e30, -1e30, -1e30);
    for (long int p = 0; p < bucketSize; p++) {
        const BakedScene::CsgOperandRecord &operand = operands[bucket[p]];
        if (operand.bounded && operand.cullSafe) {
            const Vector3Dd c = operand.bakedBounds.centroid();
            cullSafeByAxis.add(CullSafeEntry{0.0, (int)p});
            lo = Vector3Dd(
                std::fmin(lo.x(), c.x()), std::fmin(lo.y(), c.y()), std::fmin(lo.z(), c.z()));
            hi = Vector3Dd(
                std::fmax(hi.x(), c.x()), std::fmax(hi.y(), c.y()), std::fmax(hi.z(), c.z()));
        } else {
            out.alwaysTestedPositions.add((int)p);
        }
    }

    if (cullSafeByAxis.size() < kOperandCullBinThreshold) {
        out.alwaysTestedPositions.clear();
        return;
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
        const Vector3Dd c = operands[bucket[entry.position]].bakedBounds.centroid();
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

    out.binMembers.reserve(total);
    int cursor = 0;
    for (int b = 0; b < numBins; b++) {
        const int count = base + (b < remainder ? 1 : 0);
        AxisAlignedBoundingBox aggregate = AxisAlignedBoundingBox::empty();
        const int start = (int)out.binMembers.size();
        for (int k = 0; k < count; k++) {
            const int pos = cullSafeByAxis[cursor].position;
            out.binMembers.add(pos);
            aggregate = aggregate.enclosing(operands[bucket[pos]].bakedBounds);
            cursor++;
        }
        out.binBounds.add(aggregate);
        out.binMemberStart.add(start);
        out.binMemberCount.add(count);
    }
    out.built = true;
}

BakedScene::CsgProgram
BakedSceneBuilder::bakeConstructiveSolidGeometry(ConstructiveSolidGeometry *geometry, BakedScene &out)
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
    if (baked.geometryType == BooleanSetOperations::UNION) {
        BakedScene::OperandCullBins directBins;
        buildOperandCullBinsForBucket(
            baked.directPrimitiveOperandIndices, baked.operands, directBins);
        if (directBins.built) {
            BakedScene::OperandCullBins *directBinsStorage =
                new BakedScene::OperandCullBins(directBins);
            out.operandCullBinsStorage.add(directBinsStorage);
            baked.directPrimitiveCullBins = directBinsStorage;
        }
        BakedScene::OperandCullBins transformedBins;
        buildOperandCullBinsForBucket(
            baked.transformedPrimitiveOperandIndices, baked.operands, transformedBins);
        if (transformedBins.built) {
            BakedScene::OperandCullBins *transformedBinsStorage =
                new BakedScene::OperandCullBins(transformedBins);
            out.operandCullBinsStorage.add(transformedBinsStorage);
            baked.transformedPrimitiveCullBins = transformedBinsStorage;
        }
    }
    return baked;
}

int
BakedSceneBuilder::compileConstructiveSolidGeometry(ConstructiveSolidGeometry *geometry, BakedScene &out)
{
    const int index = out.csgPrograms.size();
    out.csgPrograms.add(BakedScene::CsgProgram());
    out.csgPrograms.set(index, bakeConstructiveSolidGeometry(geometry, out));
    return index;
}

BakedScene::CompositeRecord
BakedSceneBuilder::bakeComposite(Composite *object, BakedScene &out)
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

int
BakedSceneBuilder::compileTracingObject(SimpleBody *object, BakedScene &out)
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
BakedSceneBuilder::accumulateStatistics(BakedScene &scene)
{
    BakedScene::Statistics &stats = scene.statistics;
    for (long int i = 0; i < scene.traceableObjects.size(); i++) {
        const int kindIndex = (int)scene.traceableObjects[i].kind;
        if (kindIndex >= 0 && kindIndex < 6) {
            stats.countByKind[kindIndex]++;
        }
    }

    stats.topLevelObjectCount = scene.topLevelObjectIndices.size();
    for (long int i = 0; i < scene.topLevelObjectIndices.size(); i++) {
        const int objectIndex = scene.topLevelObjectIndices[i];
        const BakedScene::TraceableObject &object = scene.traceableObjects[objectIndex];
        if (object.bounded) {
            stats.topLevelObjectCullSafeCount++;
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
        if (program.geometryType == BooleanSetOperations::UNION) {
            const long int operandCount = program.operands.size();
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
            stats.unionProgramOperandHistogram[bucket]++;
        }
        for (long int j = 0; j < program.operands.size(); j++) {
            const BakedScene::CsgOperandRecord &operand = program.operands[j];
            if (program.geometryType == BooleanSetOperations::UNION) {
                stats.unionProgramOperandTotalCount++;
                if (operand.bounded && operand.cullSafe) {
                    stats.unionProgramOperandCullSafeCount++;
                }
            }
            if (operand.hasBakedQuadric) {
                stats.residualBakedQuadricOperands++;
            }
            if (operand.hasBakedPlane) {
                stats.residualBakedPlaneOperands++;
            }
            if (operand.hasTransform) {
                stats.residualTransformedOperands++;

                switch (operand.kind) {
                case BakedScene::CsgOperandKind::TransformedNestedCsg:
                    stats.residualCategory1NestedCsg++;
                    if (nestedProgramFullyBakeable(scene, operand.nestedCsgProgramIndex, 16)) {
                        stats.residualCategory1PushdownEligible++;
                    }
                    break;
                case BakedScene::CsgOperandKind::TransformedQuadric:
                case BakedScene::CsgOperandKind::TransformedPlane:
                    if (operand.operand != nullptr && operand.operand->getSteps().size() == 0) {
                        stats.residualCategory2EmptySteps++;
                    }
                    break;
                case BakedScene::CsgOperandKind::TransformedSphere:
                case BakedScene::CsgOperandKind::TransformedPrimitive:
                    stats.residualCategory3Unbakeable++;
                    break;
                default:
                    break;
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
    // Rebuilding reassigns every CsgProgram::*CullBins pointer from scratch,
    // so any previously heap-allocated OperandCullBins from an earlier
    // build() on this same BakedScene must be freed here before the owning
    // list itself is cleared - otherwise this leaks on rebuild (see
    // BakedScene::~BakedScene, which only runs once at BakedScene's own
    // end of life).
    for (long int i = 0; i < out.operandCullBinsStorage.size(); i++) {
        delete out.operandCullBinsStorage[i];
    }
    out.operandCullBinsStorage.clear();
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
