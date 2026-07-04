#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "java/lang/Math.h"
#include "common/Config.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/geometry/volume/constructiveSolidGeometry/RaySegments.h"
#include "render/bakedScene/BakedCsgTrace.h"
#include "render/bakedScene/CsgScratchContext.h"






































































bool
BakedCsgTrace::pointInsideAabb(const Vector3Dd &point, const AxisAlignedBox &box, double tolerance)
{
    return
        point.x() >= box.min.x() - tolerance &&
        point.x() <= box.max.x() + tolerance &&
        point.y() >= box.min.y() - tolerance &&
        point.y() <= box.max.y() + tolerance &&
        point.z() >= box.min.z() - tolerance &&
        point.z() <= box.max.z() + tolerance;
}

bool
BakedCsgTrace::intersectBakedPlane(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    RaySharedCache &cache,
    double *depth)
{
    Statistics &stats = *ray->getStatistics();
    stats.incrementRayPlaneTests();

    double normalDotOrigin;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getPlaneViewpointConstant(operand.planeViewpointSlot, normalDotOrigin)) {
            normalDotOrigin = operand.planeNormal.dotProduct(origin);
            normalDotOrigin += operand.planeDistance;
            normalDotOrigin *= -1.0;
            cache.setPlaneViewpointConstant(operand.planeViewpointSlot, normalDotOrigin);
        }
    } else {
        normalDotOrigin = operand.planeNormal.dotProduct(origin);
        normalDotOrigin += operand.planeDistance;
        normalDotOrigin *= -1.0;
    }

    const double normalDotDirection =
        operand.planeNormal.dotProduct(direction);
    if (normalDotDirection < Config::SMALL_TOLERANCE &&
        normalDotDirection > -Config::SMALL_TOLERANCE) {
        return false;
    }

    *depth = normalDotOrigin / normalDotDirection;
    if (*depth >= Config::SMALL_TOLERANCE &&
        *depth <= Config::MAX_DISTANCE) {
        stats.incrementRayPlaneTestsSucceeded();
        return true;
    }
    return false;
}

int
BakedCsgTrace::planeContainmentTest(
    const BakedScene::CsgOperandRecord &operand,
    const Vector3Dd &point,
    double distanceTolerance)
{
    const double signedDistance =
        point.dotProduct(operand.planeNormal) + operand.planeDistance;
    return signedDistance <= distanceTolerance ? Geometry::INSIDE : Geometry::OUTSIDE;
}

void
BakedCsgTrace::mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
{
    a = Vector3Dd(b.x() * c.y(), b.x() * c.z(), b.y() * c.z());
}

bool
BakedCsgTrace::intersectBakedQuadric(
    const Quadric &shape,
    RayWithSegments *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    bool sharesRaySpace,
    RaySharedCache &cache,
    int viewpointSlot,
    double *depth1,
    double *depth2)
{
    Statistics &stats = *ray->getStatistics();
    stats.incrementRayQuadricTests();

    Vector3Dd position2;
    Vector3Dd direction2;
    Vector3Dd positionDirection;
    Vector3Dd mixedPositionPosition;
    Vector3Dd mixedDirectionDirection;
    Vector3Dd mixedPositionDirection;
    if (sharesRaySpace) {
        // origin/direction are ray->getOrigin()/getDirection() verbatim (no
        // per-operand transform): reuse the ray's own aggregate cache
        // (RayWithSegments::makeRay, reset on every new ray generation)
        // instead of recomputing the same six vectors per operand.
        if (!ray->areQuadricConstantsCached()) {
            ray->makeRay();
        }
        position2 = ray->getPosition2();
        direction2 = ray->getDirection2();
        positionDirection = ray->getPositionDirection();
        mixedPositionPosition = ray->getMixedPositionPosition();
        mixedDirectionDirection = ray->getMixedDirectionDirection();
        mixedPositionDirection = ray->getMixedPositionDirection();
    } else {
        position2 = origin.multiply(origin);
        direction2 = direction.multiply(direction);
        positionDirection = origin.multiply(direction);
        Vector3Dd tempMixed;
        mixVectorTerms(mixedPositionPosition, origin, origin);
        mixVectorTerms(mixedDirectionDirection, direction, direction);
        mixVectorTerms(tempMixed, origin, direction);
        mixVectorTerms(mixedPositionDirection, direction, origin);
        mixedPositionDirection = mixedPositionDirection.add(tempMixed);
    }

    double squareTerm;
    if (shape.hasNonZeroSquareTerm()) {
        squareTerm = shape.getObject2Terms().dotProduct(direction2);
        squareTerm += shape.getObjectMixedTerms().dotProduct(mixedDirectionDirection);
    } else {
        squareTerm = 0.0;
    }

    double linearTerm = shape.getObject2Terms().dotProduct(positionDirection);
    linearTerm *= 2.0;
    linearTerm += shape.getObjectTerms().dotProduct(direction);
    linearTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionDirection);

    double constantTermWithoutMixed;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed)) {
            constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
            constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
            constantTermWithoutMixed += shape.getObjectConstant();
            cache.setQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed);
        }
    } else {
        constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
        constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
        constantTermWithoutMixed += shape.getObjectConstant();
    }
    double constantTerm = constantTermWithoutMixed;
    constantTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionPosition);

    if (squareTerm != 0.0) {
        const double determinant2 =
            linearTerm * linearTerm - 4.0 * squareTerm * constantTerm;
        if (determinant2 < 0.0) {
            return false;
        }

        const double determinant = java::Math::sqrt(determinant2);
        const double a2 = squareTerm * 2.0;
        const double bMinus = linearTerm * -1.0;
        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        if (linearTerm == 0.0) {
            return false;
        }
        *depth1 = constantTerm * -1.0 / linearTerm;
        *depth2 = *depth1;
    }

    if ((*depth1 < Config::SMALL_TOLERANCE) ||
        (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) ||
            (*depth2 > Config::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < Config::SMALL_TOLERANCE) ||
               (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return true;
}

// Like intersectBakedQuadric but also sets trueMiss=true when the discriminant indicates the
// ray definitively misses the quadric (safe to skip plane candidate checks). Intended for the
// hot path in traceTransformedNestedSingleCorePlaneOperandAllCrossings, where the three
// polynomial coefficients (A/B/C) of intersectBakedQuadricWithCoeffs are not needed and
// would add register pressure on every call.
bool
BakedCsgTrace::intersectBakedQuadricWithTrueMiss(
    const Quadric &shape,
    RayWithSegments *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    bool sharesRaySpace,
    RaySharedCache &cache,
    int viewpointSlot,
    double *depth1,
    double *depth2,
    bool &trueMiss)
{
    Statistics &stats = *ray->getStatistics();
    stats.incrementRayQuadricTests();

    Vector3Dd position2;
    Vector3Dd direction2;
    Vector3Dd positionDirection;
    Vector3Dd mixedPositionPosition;
    Vector3Dd mixedDirectionDirection;
    Vector3Dd mixedPositionDirection;
    if (sharesRaySpace) {
        if (!ray->areQuadricConstantsCached()) {
            ray->makeRay();
        }
        position2 = ray->getPosition2();
        direction2 = ray->getDirection2();
        positionDirection = ray->getPositionDirection();
        mixedPositionPosition = ray->getMixedPositionPosition();
        mixedDirectionDirection = ray->getMixedDirectionDirection();
        mixedPositionDirection = ray->getMixedPositionDirection();
    } else {
        position2 = origin.multiply(origin);
        direction2 = direction.multiply(direction);
        positionDirection = origin.multiply(direction);
        Vector3Dd tempMixed;
        mixVectorTerms(mixedPositionPosition, origin, origin);
        mixVectorTerms(mixedDirectionDirection, direction, direction);
        mixVectorTerms(tempMixed, origin, direction);
        mixVectorTerms(mixedPositionDirection, direction, origin);
        mixedPositionDirection = mixedPositionDirection.add(tempMixed);
    }

    trueMiss = false;

    double squareTerm;
    if (shape.hasNonZeroSquareTerm()) {
        squareTerm = shape.getObject2Terms().dotProduct(direction2);
        squareTerm += shape.getObjectMixedTerms().dotProduct(mixedDirectionDirection);
    } else {
        squareTerm = 0.0;
    }

    double linearTerm = shape.getObject2Terms().dotProduct(positionDirection);
    linearTerm *= 2.0;
    linearTerm += shape.getObjectTerms().dotProduct(direction);
    linearTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionDirection);

    double constantTermWithoutMixed;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed)) {
            constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
            constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
            constantTermWithoutMixed += shape.getObjectConstant();
            cache.setQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed);
        }
    } else {
        constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
        constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
        constantTermWithoutMixed += shape.getObjectConstant();
    }
    double constantTerm = constantTermWithoutMixed;
    constantTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionPosition);

    if (squareTerm != 0.0) {
        const double determinant2 = linearTerm * linearTerm - 4.0 * squareTerm * constantTerm;
        if (determinant2 < 0.0) {
            trueMiss = determinant2 < -4.0 * squareTerm * Config::SMALL_TOLERANCE;
            return false;
        }
        const double determinant = java::Math::sqrt(determinant2);
        const double a2 = squareTerm * 2.0;
        const double bMinus = linearTerm * -1.0;
        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        if (linearTerm == 0.0) {
            return false;
        }
        *depth1 = constantTerm * -1.0 / linearTerm;
        *depth2 = *depth1;
    }

    if ((*depth1 < Config::SMALL_TOLERANCE) || (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return true;
}

// Like intersectBakedQuadric but also outputs the quadric polynomial coefficients A/B/C
// (where Q(origin + t*direction) = A*t^2 + B*t + C) and sets trueMiss=true when the
// discriminant is negative (ray definitively misses the quadric - safe to skip plane checks).
// When trueMiss=false and the function returns false, the ray may be inside the quadric.
bool
BakedCsgTrace::intersectBakedQuadricWithCoeffs(
    const Quadric &shape,
    RayWithSegments *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    bool sharesRaySpace,
    RaySharedCache &cache,
    int viewpointSlot,
    double *depth1,
    double *depth2,
    double &polyA,
    double &polyB,
    double &polyC,
    bool &trueMiss)
{
    Statistics &stats = *ray->getStatistics();
    stats.incrementRayQuadricTests();

    Vector3Dd position2;
    Vector3Dd direction2;
    Vector3Dd positionDirection;
    Vector3Dd mixedPositionPosition;
    Vector3Dd mixedDirectionDirection;
    Vector3Dd mixedPositionDirection;
    if (sharesRaySpace) {
        if (!ray->areQuadricConstantsCached()) {
            ray->makeRay();
        }
        position2 = ray->getPosition2();
        direction2 = ray->getDirection2();
        positionDirection = ray->getPositionDirection();
        mixedPositionPosition = ray->getMixedPositionPosition();
        mixedDirectionDirection = ray->getMixedDirectionDirection();
        mixedPositionDirection = ray->getMixedPositionDirection();
    } else {
        position2 = origin.multiply(origin);
        direction2 = direction.multiply(direction);
        positionDirection = origin.multiply(direction);
        Vector3Dd tempMixed;
        mixVectorTerms(mixedPositionPosition, origin, origin);
        mixVectorTerms(mixedDirectionDirection, direction, direction);
        mixVectorTerms(tempMixed, origin, direction);
        mixVectorTerms(mixedPositionDirection, direction, origin);
        mixedPositionDirection = mixedPositionDirection.add(tempMixed);
    }

    trueMiss = false;

    if (shape.hasNonZeroSquareTerm()) {
        polyA = shape.getObject2Terms().dotProduct(direction2);
        polyA += shape.getObjectMixedTerms().dotProduct(mixedDirectionDirection);
    } else {
        polyA = 0.0;
    }

    polyB = shape.getObject2Terms().dotProduct(positionDirection);
    polyB *= 2.0;
    polyB += shape.getObjectTerms().dotProduct(direction);
    polyB += shape.getObjectMixedTerms().dotProduct(mixedPositionDirection);

    double polyCWithoutMixed;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getQuadricViewpointConstant(viewpointSlot, polyCWithoutMixed)) {
            polyCWithoutMixed = shape.getObject2Terms().dotProduct(position2);
            polyCWithoutMixed += shape.getObjectTerms().dotProduct(origin);
            polyCWithoutMixed += shape.getObjectConstant();
            cache.setQuadricViewpointConstant(viewpointSlot, polyCWithoutMixed);
        }
    } else {
        polyCWithoutMixed = shape.getObject2Terms().dotProduct(position2);
        polyCWithoutMixed += shape.getObjectTerms().dotProduct(origin);
        polyCWithoutMixed += shape.getObjectConstant();
    }
    polyC = polyCWithoutMixed;
    polyC += shape.getObjectMixedTerms().dotProduct(mixedPositionPosition);

    if (polyA != 0.0) {
        const double determinant2 = polyB * polyB - 4.0 * polyA * polyC;
        if (determinant2 < 0.0) {
            // Safe to skip plane-candidate containment checks only when the ray misses
            // the quadric by more than SMALL_TOLERANCE. The containment test accepts
            // points with quadric-value < SMALL_TOLERANCE; the minimum quadric value
            // along the ray is -discriminant2/(4*polyA), so trueMiss is safe exactly
            // when -discriminant2/(4*polyA) > SMALL_TOLERANCE, i.e.
            // discriminant2 < -4*polyA*SMALL_TOLERANCE.
            trueMiss = determinant2 < -4.0 * polyA * Config::SMALL_TOLERANCE;
            return false;
        }

        const double determinant = java::Math::sqrt(determinant2);
        const double a2 = polyA * 2.0;
        const double bMinus = polyB * -1.0;
        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        if (polyB == 0.0) {
            return false;
        }
        *depth1 = polyC * -1.0 / polyB;
        *depth2 = *depth1;
    }

    if ((*depth1 < Config::SMALL_TOLERANCE) ||
        (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) ||
            (*depth2 > Config::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < Config::SMALL_TOLERANCE) ||
               (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return true;
}

void
BakedCsgTrace::offerTransformedPrimitiveCandidate(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *ray,
    Material *effectiveMaterial,
    const Vector3Dd &localOrigin,
    const Vector3Dd &localDirection,
    double depth,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    IntersectionCandidate candidate;
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        operand.objectToLocal.transformPoint(candidate.getIntersection().point);
    const Vector3Dd rayOrigin = ray->getOrigin();
    const Vector3Dd rayDir = ray->getDirection();
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);
    depthQueue->offer(candidate);
}

bool
BakedCsgTrace::annotateDirectCandidates(
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const BakedScene::CsgOperandRecord &operand)
{
    bool annotated = false;
    // The direct-to-destination fast path is only used for non-transformed,
    // non-nested primitive geometry. Existing queue entries have already been
    // annotated by this helper; fresh primitive hits are the only ones still
    // carrying an empty detail-owner stack and the default point-space flag.
    for (IntersectionCandidate &candidate : *depthQueue) {
        IntersectionAttributes &attributes = candidate.getAttributes();
        if (attributes.getDetailOwnerCount() != 0 ||
            attributes.getMaterialUsesObjectLocalPoint()) {
            continue;
        }
        attributes.pushDetailOwner(operand.operand);
        attributes.setMaterialUsesObjectLocalPoint(true);
        annotated = true;
    }
    return annotated;
}

int
BakedCsgTrace::containmentTestOperand(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    switch (operand.kind) {
    case BakedScene::CsgOperandKind::Empty:
        return Geometry::OUTSIDE;

    case BakedScene::CsgOperandKind::DirectPlane:
        return planeContainmentTest(operand, point, distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedPlane:
        return planeContainmentTest(
            operand,
            operand.localToObject.transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::NestedCsg:
        return BakedCsgTrace::containmentTest(
            bakedCsgs[operand.nestedCsgProgramIndex],
            bakedCsgs,
            point,
            distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedNestedCsg:
        return BakedCsgTrace::containmentTest(
            bakedCsgs[operand.nestedCsgProgramIndex],
            bakedCsgs,
            operand.localToObject.transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedQuadric:
    case BakedScene::CsgOperandKind::TransformedSphere:
    case BakedScene::CsgOperandKind::TransformedPrimitive:
        return operand.geometry->doContainmentTest(
            operand.localToObject.transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
    case BakedScene::CsgOperandKind::DirectPrimitive:
    case BakedScene::CsgOperandKind::GenericFallback:
        return operand.geometry != nullptr ?
            operand.geometry->doContainmentTest(point, distanceTolerance) :
            Geometry::OUTSIDE;
    }
    return Geometry::OUTSIDE;
}

bool
BakedCsgTrace::tracePlanOperandAllCrossings(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (operand.compiledTransformedNestedCorePlane &&
        operand.nestedCsgProgramIndex >= 0 &&
        operand.nestedCsgProgramIndex < bakedCsgs.size() &&
        !ray->isPrimaryRayEnabled()) {
        Material *effectiveMaterial =
            operand.material != nullptr ? operand.material : materialOverride;
        return traceTransformedNestedSingleCorePlaneOperandAllCrossings(
            operand,
            bakedCsgs[operand.nestedCsgProgramIndex],
            bakedCsgs,
            ray,
            depthQueue,
            scratch.getCache(),
            effectiveMaterial);
    }

    return traceOperandAllCrossings(
        operand,
        bakedCsgs,
        scratch,
        ray,
        depthQueue,
        materialOverride);
}

RaySegments
BakedCsgTrace::buildRaySegments(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();

    tracePlanOperandAllCrossings(
        operand,
        bakedCsgs,
        scratch,
        ray,
        localDepthQueue,
        materialOverride);

    bool initialInside;
    if (localDepthQueue->size() > 0) {
        IntersectionCandidate firstCandidate = localDepthQueue->peek();
        Vector3Dd samplePoint = ray->getOrigin().add(
            ray->getDirection().multiply(0.5 * firstCandidate.getIntersection().t));
        initialInside =
            containmentTestOperand(
                operand, bakedCsgs, samplePoint, 0.0) == Geometry::INSIDE;
    } else {
        Vector3Dd samplePoint =
            ray->getOrigin().add(ray->getDirection().multiply(2.0 * Config::SMALL_TOLERANCE));
        initialInside =
            containmentTestOperand(
                operand, bakedCsgs, samplePoint, 0.0) == Geometry::INSIDE;
    }

    java::ArrayList<RaySegmentCrossing> crossings{localDepthQueue->size()};
    bool currentlyInside = initialInside;
    while (localDepthQueue->size() > 0) {
        IntersectionCandidate candidate = localDepthQueue->poll();
        currentlyInside = !currentlyInside;
        crossings.add(RaySegmentCrossing(candidate.getIntersection().t, currentlyInside, candidate));
    }

    scratch.returnQueue(localDepthQueue);
    return RaySegments(crossings, initialInside);
}

bool
BakedCsgTrace::combineUnion(bool insideLeft, bool insideRight)
{
    return insideLeft || insideRight;
}

bool
BakedCsgTrace::combineIntersection(bool insideLeft, bool insideRight)
{
    return insideLeft && insideRight;
}

bool
BakedCsgTrace::combineDifference(bool insideLeft, bool insideRight)
{
    return insideLeft && !insideRight;
}

RaySegments
BakedCsgTrace::mergeByMembership(
    const RaySegments &left,
    const RaySegments &right,
    bool (*combine)(bool insideLeft, bool insideRight))
{
    const java::ArrayList<RaySegmentCrossing> &leftCrossings = left.getCrossings();
    const java::ArrayList<RaySegmentCrossing> &rightCrossings = right.getCrossings();

    java::ArrayList<RaySegmentCrossing> outCrossings{leftCrossings.size() + rightCrossings.size()};
    bool insideLeft = left.isInitialInside();
    bool insideRight = right.isInitialInside();
    const bool initialCombined = combine(insideLeft, insideRight);

    bool previousCombined = initialCombined;
    long int i = 0;
    long int j = 0;
    while ((i < leftCrossings.size()) || (j < rightCrossings.size())) {
        bool takeLeft;
        if (j >= rightCrossings.size()) {
            takeLeft = true;
        } else if (i >= leftCrossings.size()) {
            takeLeft = false;
        } else {
            takeLeft = leftCrossings[i].getT() <= rightCrossings[j].getT();
        }

        RaySegmentCrossing event;
        if (takeLeft) {
            event = leftCrossings[i];
            insideLeft = event.isEntering();
            i++;
        } else {
            event = rightCrossings[j];
            insideRight = event.isEntering();
            j++;
        }

        const bool newCombined = combine(insideLeft, insideRight);
        if (newCombined != previousCombined) {
            outCrossings.add(RaySegmentCrossing(event.getT(), newCombined, event.getHit()));
            previousCombined = newCombined;
        }
    }
    return RaySegments(outCrossings, initialCombined);
}

int
BakedCsgTrace::traceRaySegmentCsg(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.operands.size() == 0) {
        return false;
    }

    if (bakedCsg.planKind ==
            BakedScene::CsgPlanKind::TopLevelPlaneUnion ||
        bakedCsg.planKind ==
            BakedScene::CsgPlanKind::DisjointBoundedUnion) {
        bool anyFound = false;
        for (long int i = 0; i < bakedCsg.operands.size(); i++) {
            if (tracePlanOperandAllCrossings(
                    bakedCsg.operands[i],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }
        return anyFound;
    }

    if (bakedCsg.planKind ==
            BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
        bakedCsg.specializationValid) {
        return traceSingleCorePlaneIntersection(
            bakedCsg,
            bakedCsgs,
            scratch,
            ray,
            depthQueue,
            materialOverride,
            bakedCsg.specializationCoreOperandIndex);
    }

    RaySegments result = buildRaySegments(
        bakedCsg.operands[0], bakedCsgs, scratch, ray, materialOverride);
    for (long int i = 1; i < bakedCsg.operands.size(); i++) {
        const RaySegments childSegments =
            buildRaySegments(
                bakedCsg.operands[i], bakedCsgs, scratch, ray, materialOverride);
        switch (bakedCsg.geometryType) {
        case BooleanSetOperations::DIFFERENCE:
            result = mergeByMembership(result, childSegments, combineDifference);
            break;
        case BooleanSetOperations::INTERSECTION:
            result = mergeByMembership(result, childSegments, combineIntersection);
            break;
        default:
            result = mergeByMembership(result, childSegments, combineUnion);
            break;
        }
    }

    bool intersectionFound = false;
    const java::ArrayList<RaySegmentCrossing> &resultCrossings = result.getCrossings();
    for (long int i = 0; i < resultCrossings.size(); i++) {
        depthQueue->offer(resultCrossings[i].getHit());
        intersectionFound = true;
    }
    return intersectionFound;
}

bool
BakedCsgTrace::candidateInsideAllOtherOperands(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex)
{
    for (long int j = bakedCsg.operands.size() - 1; j >= 0; j--) {
        if (j == skipIndex) {
            continue;
        }
        if (containmentTestOperand(
                bakedCsg.operands[j],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
BakedCsgTrace::candidateInsideOperandsCoreFirst(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex,
    long int coreIndex)
{
    if (coreIndex >= 0 && coreIndex != skipIndex) {
        if (containmentTestOperand(
                bakedCsg.operands[coreIndex],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    for (long int j = bakedCsg.operands.size() - 1; j >= 0; j--) {
        if (j == skipIndex || j == coreIndex) {
            continue;
        }
        if (containmentTestOperand(
                bakedCsg.operands[j],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
BakedCsgTrace::candidateInsideCompiledSingleCorePlaneOperands(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex,
    long int coreIndex)
{
    if (coreIndex >= 0 && coreIndex != skipIndex) {
        if (containmentTestOperand(
                bakedCsg.operands[coreIndex],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    for (long int p = bakedCsg.planeOperandIndices.size() - 1;
         p >= 0; p--) {
        const int operandIndex = bakedCsg.planeOperandIndices[p];
        if (operandIndex == skipIndex) {
            continue;
        }
        if (containmentTestOperand(
                bakedCsg.operands[operandIndex],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
BakedCsgTrace::candidateInsideCompiledNestedContainmentSequence(
    const BakedScene::CsgOperandRecord &parentOperand,
    const BakedScene::CsgProgram &nestedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex)
{
    for (long int s =
             parentOperand.compiledNestedContainmentOperandIndices.size() - 1;
         s >= 0; s--) {
        const int operandIndex =
            parentOperand.compiledNestedContainmentOperandIndices[s];
        if (operandIndex == skipIndex) {
            continue;
        }
        if (containmentTestOperand(
                nestedCsg.operands[operandIndex],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

// Inlined containment check for the compiled direct-quadric core + direct-plane descriptor.
// Skips the containmentTestOperand switch for each check.
bool
BakedCsgTrace::candidateInsideDirectDescriptorOperands(
    const BakedScene::CsgOperandRecord &parentOperand,
    const BakedScene::CsgProgram &nestedCsg,
    const Vector3Dd &point,
    long int skipIndex)
{
    // Check planes in reverse order (same order as the compiled containment sequence
    // which stores [coreIndex, plane1Index, ...] iterated from the end).
    for (long int p = parentOperand.compiledNestedPlaneOperandIndices.size() - 1;
         p >= 0; p--) {
        const int planeIdx = parentOperand.compiledNestedPlaneOperandIndices[p];
        if (planeIdx == skipIndex) {
            continue;
        }
        if (planeContainmentTest(
                nestedCsg.operands[planeIdx],
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    // Check core quadric last (it is stored first in the containment sequence, so
    // would be checked last in a reversed iteration).
    const long int coreIndex = parentOperand.compiledNestedCoreOperandIndex;
    if (skipIndex != coreIndex) {
        Quadric *quadric = nestedCsg.operands[coreIndex].quadricGeometry;
        if (quadric == nullptr ||
            quadric->doContainmentTest(point, Config::SMALL_TOLERANCE) ==
                Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
BakedCsgTrace::tracePlaneOperandCandidate(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *ray,
    RaySharedCache &cache,
    Material *materialOverride,
    IntersectionCandidate &candidate)
{
    if (!operand.isInfinitePlane || operand.nestedCsgProgramIndex >= 0 ||
        operand.geometry == nullptr) {
        return false;
    }

    Material *effectiveMaterial =
        operand.material != nullptr ? operand.material : materialOverride;
    if (operand.hasTransform) {
        const Vector3Dd localOrigin =
            operand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand.localToObject.transformDirection(ray->getDirection());

        double depth;
        if (!intersectBakedPlane(
                operand,
                ray,
                localOrigin,
                localDirection,
                cache,
                &depth) ||
            depth <= Config::SMALL_TOLERANCE) {
            return false;
        }

        candidate = IntersectionCandidate();
        candidate.getIntersection().point =
            localOrigin.add(localDirection.multiply(depth));
        candidate.getAttributes().setHitGeometry(operand.geometry);
        candidate.getAttributes().setMaterial(effectiveMaterial);
        candidate.getAttributes().pushDetailOwner(operand.operand);
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
        candidate.getIntersection().point =
            operand.objectToLocal.transformPoint(candidate.getIntersection().point);
        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        candidate.getIntersection().t =
            candidate.getIntersection().point
                .subtract(rayOrigin).dotProduct(rayDir) /
            rayDir.dotProduct(rayDir);
        return true;
    }

    double depth;
    if (!intersectBakedPlane(
            operand,
            ray,
            ray->getOrigin(),
            ray->getDirection(),
            cache,
            &depth) ||
        depth <= Config::SMALL_TOLERANCE) {
        return false;
    }

    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        ray->getOrigin().add(ray->getDirection().multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().t = depth;
    return true;
}

bool
BakedCsgTrace::tracePlaneOperandCandidateInRaySpace(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *statsRay,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    RaySharedCache &cache,
    Material *materialOverride,
    IntersectionCandidate &candidate)
{
    if (!operand.isInfinitePlane || operand.nestedCsgProgramIndex >= 0 ||
        operand.geometry == nullptr) {
        return false;
    }

    Material *effectiveMaterial =
        operand.material != nullptr ? operand.material : materialOverride;
    Vector3Dd localOrigin = rayOrigin;
    Vector3Dd localDirection = rayDirection;
    if (operand.hasTransform) {
        localOrigin = operand.localToObject.transformPoint(rayOrigin);
        localDirection = operand.localToObject.transformDirection(rayDirection);
    }

    double depth;
    if (!intersectBakedPlane(
            operand,
            statsRay,
            localOrigin,
            localDirection,
            cache,
            &depth) ||
        depth <= Config::SMALL_TOLERANCE) {
        return false;
    }

    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    if (operand.hasTransform) {
        candidate.getIntersection().point =
            operand.objectToLocal.transformPoint(candidate.getIntersection().point);
    }
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDirection) /
        rayDirection.dotProduct(rayDirection);
    return true;
}

bool
BakedCsgTrace::traceCompiledCoreOperandAllCrossings(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (operand.geometry == nullptr) {
        return false;
    }
    if (operand.bounded && operand.cullSafe &&
        !rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
        return false;
    }

    Material *effectiveMaterial =
        operand.material != nullptr ? operand.material : materialOverride;

    switch (operand.kind) {
    case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
    {
        GeometryIntersectionEmissionContext context;
        context.materialOverride = effectiveMaterial;
        context.detailOwner = operand.operand;
        context.materialUsesObjectLocalPoint = true;
        return operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
            ray, depthQueue, context);
    }

    case BakedScene::CsgOperandKind::DirectPrimitive:
    {
        const int initialSize = depthQueue->size();
        const bool found = operand.geometry->doIntersectionForAllRayCrossings(
            ray, depthQueue, effectiveMaterial);
        if (!found || depthQueue->size() == initialSize) {
            return false;
        }
        return annotateDirectCandidates(depthQueue, operand);
    }

    case BakedScene::CsgOperandKind::TransformedQuadric:
    {
        const Vector3Dd localOrigin =
            operand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand.localToObject.transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (operand.quadricGeometry == nullptr) {
            return false;
        }
        if (!intersectBakedQuadric(
                *operand.quadricGeometry,
                ray,
                localOrigin,
                localDirection,
                false,
                scratch.getCache(),
                operand.quadricViewpointSlot,
                &depth1,
                &depth2)) {
            return false;
        }

        offerTransformedPrimitiveCandidate(
            operand,
            ray,
            effectiveMaterial,
            localOrigin,
            localDirection,
            depth1,
            depthQueue);
        if (depth2 != depth1) {
            offerTransformedPrimitiveCandidate(
                operand,
                ray,
                effectiveMaterial,
                localOrigin,
                localDirection,
                depth2,
                depthQueue);
        }
        return true;
    }

    case BakedScene::CsgOperandKind::TransformedSphere:
    {
        const Vector3Dd localOrigin =
            operand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand.localToObject.transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (!Sphere::intersectSphereLocalSpace(
                localOrigin, localDirection,
                ray->getStatistics(), &depth1, &depth2)) {
            return false;
        }

        offerTransformedPrimitiveCandidate(
            operand, ray, effectiveMaterial,
            localOrigin, localDirection, depth1, depthQueue);
        if (depth2 != depth1) {
            offerTransformedPrimitiveCandidate(
                operand, ray, effectiveMaterial,
                localOrigin, localDirection, depth2, depthQueue);
        }
        return true;
    }

    default:
        return traceOperandAllCrossings(
            operand,
            bakedCsgs,
            scratch,
            ray,
            depthQueue,
            materialOverride);
    }
}

bool
BakedCsgTrace::canUseCompiledSingleCorePlanePlan(
    const BakedScene::CsgProgram &bakedCsg,
    long int coreIndex)
{
    return
        bakedCsg.planKind ==
            BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
        bakedCsg.specializationValid &&
        coreIndex >= 0 &&
        coreIndex < bakedCsg.operands.size() &&
        bakedCsg.planeOperandIndices.size() + 1 ==
            bakedCsg.operands.size();
}

bool
BakedCsgTrace::offerTransformedQuadricCoreCandidate(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const BakedScene::CsgOperandRecord &coreOperand,
    RayWithSegments *ray,
    Material *effectiveMaterial,
    const Vector3Dd &localOrigin,
    const Vector3Dd &localDirection,
    double depth,
    long int coreIndex,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
        return false;
    }

    IntersectionCandidate candidate;
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(coreOperand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(coreOperand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        coreOperand.objectToLocal.transformPoint(candidate.getIntersection().point);
    const Vector3Dd rayOrigin = ray->getOrigin();
    const Vector3Dd rayDir = ray->getDirection();
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);

    if (!candidateInsideCompiledSingleCorePlaneOperands(
            bakedCsg,
            bakedCsgs,
            candidate.getIntersection().point,
            coreIndex,
            coreIndex)) {
        return false;
    }
    depthQueue->offer(candidate);
    return true;
}

bool
BakedCsgTrace::traceTransformedQuadricCorePlaneIntersection(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride,
    long int coreIndex,
    bool &coreTrueMiss)
{
    coreTrueMiss = false;
    const BakedScene::CsgOperandRecord &coreOperand = bakedCsg.operands[coreIndex];
    if (coreOperand.kind !=
        BakedScene::CsgOperandKind::TransformedQuadric) {
        return false;
    }
    if (coreOperand.geometry == nullptr) {
        return false;
    }
    if (coreOperand.bounded && coreOperand.cullSafe &&
        !rayIntersectsAabbForward(*ray, coreOperand.bakedBounds)) {
        return false;
    }

    Material *effectiveMaterial =
        coreOperand.material != nullptr ? coreOperand.material : materialOverride;
    const Vector3Dd localOrigin =
        coreOperand.localToObject.transformPoint(ray->getOrigin());
    const Vector3Dd localDirection =
        coreOperand.localToObject.transformDirection(ray->getDirection());

    double depth1;
    double depth2;
    if (coreOperand.quadricGeometry == nullptr) {
        return false;
    }
    if (!intersectBakedQuadricWithTrueMiss(
            *coreOperand.quadricGeometry,
            ray,
            localOrigin,
            localDirection,
            false,
            cache,
            coreOperand.quadricViewpointSlot,
            &depth1,
            &depth2,
            coreTrueMiss)) {
        return false;
    }

    bool anyIntersectionFound = false;
    if (offerTransformedQuadricCoreCandidate(
            bakedCsg,
            bakedCsgs,
            coreOperand,
            ray,
            effectiveMaterial,
            localOrigin,
            localDirection,
            depth1,
            coreIndex,
            depthQueue)) {
        anyIntersectionFound = true;
    }
    if (depth2 != depth1 &&
        offerTransformedQuadricCoreCandidate(
            bakedCsg,
            bakedCsgs,
            coreOperand,
            ray,
            effectiveMaterial,
            localOrigin,
            localDirection,
            depth2,
            coreIndex,
            depthQueue)) {
        anyIntersectionFound = true;
    }
    return anyIntersectionFound;
}

void
BakedCsgTrace::emitNestedCandidateToParentOperand(
    const BakedScene::CsgOperandRecord &parentOperand,
    RayWithSegments *parentRay,
    const Matrix4x4d &nestedToParent,
    IntersectionCandidate &candidate,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    candidate.getAttributes().pushDetailOwner(parentOperand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        nestedToParent.transformPoint(candidate.getIntersection().point);
    const Vector3Dd rayOrigin = parentRay->getOrigin();
    const Vector3Dd rayDir = parentRay->getDirection();
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);
    depthQueue->offer(candidate);
}

bool
BakedCsgTrace::makeTransformedQuadricCandidateInRaySpace(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *statsRay,
    Material *effectiveMaterial,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
        return false;
    }
    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        rayOrigin.add(rayDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        operand.objectToLocal.transformPoint(candidate.getIntersection().point);
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(statsRay->getOrigin())
            .dotProduct(statsRay->getDirection()) /
        statsRay->getDirection().dotProduct(statsRay->getDirection());
    return true;
}

bool
BakedCsgTrace::makeDirectQuadricCandidateInRaySpace(
    const BakedScene::CsgOperandRecord &operand,
    Material *effectiveMaterial,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
        return false;
    }
    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        rayOrigin.add(rayDirection.multiply(depth));
    candidate.getIntersection().t = depth;
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    return true;
}

bool
BakedCsgTrace::traceTransformedNestedSingleCorePlaneOperandAllCrossings(
    const BakedScene::CsgOperandRecord &parentOperand,
    const BakedScene::CsgProgram &nestedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    RayWithSegments *parentRay,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride)
{
    const long int coreIndex = parentOperand.compiledNestedCoreOperandIndex;
    if (!canUseCompiledSingleCorePlanePlan(nestedCsg, coreIndex)) {
        return false;
    }
    const BakedScene::CsgOperandRecord &coreOperand = nestedCsg.operands[coreIndex];
    Quadric *directCoreQuadric =
        coreOperand.kind ==
            BakedScene::CsgOperandKind::DirectAnnotatedPrimitive ?
        coreOperand.quadricGeometry :
        nullptr;
    const bool transformedCoreQuadric =
        coreOperand.kind ==
            BakedScene::CsgOperandKind::TransformedQuadric;
    if (!transformedCoreQuadric && directCoreQuadric == nullptr) {
        return false;
    }

    const Vector3Dd nestedRayOrigin =
        parentOperand.localToObject.transformPoint(parentRay->getOrigin());
    const Vector3Dd nestedRayDirection =
        parentOperand.localToObject.transformDirection(parentRay->getDirection());

    Material *effectiveCoreMaterial =
        coreOperand.material != nullptr ? coreOperand.material : materialOverride;
    const Vector3Dd coreRayOrigin = transformedCoreQuadric ?
        coreOperand.localToObject.transformPoint(nestedRayOrigin) :
        nestedRayOrigin;
    const Vector3Dd coreRayDirection = transformedCoreQuadric ?
        coreOperand.localToObject.transformDirection(nestedRayDirection) :
        nestedRayDirection;

    bool anyIntersectionFound = false;
    double depth1;
    double depth2;

    if (directCoreQuadric != nullptr) {
        // Direct-quadric path: use intersectBakedQuadricWithTrueMiss to detect ray misses
        // early and skip plane candidate checks (~89% of calls for drum shadow rays miss
        // the cylinder). intersectBakedQuadricWithCoeffs is intentionally NOT used here:
        // it outputs polyA/polyB/polyC that are unused in this emitter, adding register
        // pressure on every call. intersectBakedQuadricWithTrueMiss provides only the
        // trueMiss flag without the unused output parameters.
        // Plan 8 R2: a pushdown-folded wrapper has identity matrices, so
        // coreRayOrigin/coreRayDirection above are bit-equal to the parent
        // ray's own origin/direction - the ray's cached aggregates apply.
        bool trueMiss = false;
        const bool quadricHit = intersectBakedQuadricWithTrueMiss(
            *directCoreQuadric,
            parentRay,
            coreRayOrigin,
            coreRayDirection,
            parentOperand.pushdownFolded,
            cache,
            coreOperand.quadricViewpointSlot,
            &depth1,
            &depth2,
            trueMiss);

        if (trueMiss) {
            return false;
        }

        if (quadricHit) {
            IntersectionCandidate candidate;
            const bool madeDepth1 = makeDirectQuadricCandidateInRaySpace(
                coreOperand, effectiveCoreMaterial, coreRayOrigin,
                coreRayDirection, depth1, candidate);
            if (madeDepth1 &&
                candidateInsideDirectDescriptorOperands(
                    parentOperand, nestedCsg,
                    candidate.getIntersection().point,
                    coreIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand.objectToLocal,
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
            if (depth2 != depth1) {
                const bool madeDepth2 = makeDirectQuadricCandidateInRaySpace(
                    coreOperand, effectiveCoreMaterial, coreRayOrigin,
                    coreRayDirection, depth2, candidate);
                if (madeDepth2 &&
                    candidateInsideDirectDescriptorOperands(
                        parentOperand, nestedCsg,
                        candidate.getIntersection().point,
                        coreIndex)) {
                    emitNestedCandidateToParentOperand(
                        parentOperand, parentRay, parentOperand.objectToLocal,
                        candidate, depthQueue);
                    anyIntersectionFound = true;
                }
            }
        }

        for (long int p = parentOperand.compiledNestedPlaneOperandIndices.size() - 1;
             p >= 0; p--) {
            const int operandIndex = parentOperand.compiledNestedPlaneOperandIndices[p];
            const BakedScene::CsgOperandRecord &operand = nestedCsg.operands[operandIndex];
            IntersectionCandidate candidate;
            if (tracePlaneOperandCandidateInRaySpace(
                    operand, parentRay,
                    nestedRayOrigin, nestedRayDirection,
                    cache, materialOverride, candidate) &&
                candidateInsideDirectDescriptorOperands(
                    parentOperand, nestedCsg,
                    candidate.getIntersection().point,
                    operandIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand.objectToLocal,
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
        }
    } else {
        // Transformed-quadric path: keep original code path.
        if (intersectBakedQuadric(
                *coreOperand.quadricGeometry,
                parentRay,
                coreRayOrigin,
                coreRayDirection,
                false,
                cache,
                coreOperand.quadricViewpointSlot,
                &depth1,
                &depth2)) {
            IntersectionCandidate candidate;
            const bool madeDepth1 = makeTransformedQuadricCandidateInRaySpace(
                coreOperand, parentRay, effectiveCoreMaterial, coreRayOrigin,
                coreRayDirection, depth1, candidate);
            if (madeDepth1 &&
                candidateInsideCompiledNestedContainmentSequence(
                    parentOperand, nestedCsg, bakedCsgs,
                    candidate.getIntersection().point, coreIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand.objectToLocal,
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
            if (depth2 != depth1) {
                const bool madeDepth2 = makeTransformedQuadricCandidateInRaySpace(
                    coreOperand, parentRay, effectiveCoreMaterial, coreRayOrigin,
                    coreRayDirection, depth2, candidate);
                if (madeDepth2 &&
                    candidateInsideCompiledNestedContainmentSequence(
                        parentOperand, nestedCsg, bakedCsgs,
                        candidate.getIntersection().point, coreIndex)) {
                    emitNestedCandidateToParentOperand(
                        parentOperand, parentRay, parentOperand.objectToLocal,
                        candidate, depthQueue);
                    anyIntersectionFound = true;
                }
            }
        }

        for (long int p = parentOperand.compiledNestedPlaneOperandIndices.size() - 1;
             p >= 0; p--) {
            const int operandIndex = parentOperand.compiledNestedPlaneOperandIndices[p];
            const BakedScene::CsgOperandRecord &operand = nestedCsg.operands[operandIndex];
            IntersectionCandidate candidate;
            if (tracePlaneOperandCandidateInRaySpace(
                    operand, parentRay,
                    nestedRayOrigin, nestedRayDirection,
                    cache, materialOverride, candidate) &&
                candidateInsideCompiledNestedContainmentSequence(
                    parentOperand, nestedCsg, bakedCsgs,
                    candidate.getIntersection().point, operandIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand.objectToLocal,
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
        }
    }

    return anyIntersectionFound;
}

int
BakedCsgTrace::traceSingleCorePlaneIntersection(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    long int coreIndex)
{
    if (!bakedCsg.specializationValid) {
        return false;
    }

    bool anyIntersectionFound = false;
    bool coreTrueMiss = false;
    const BakedScene::CsgOperandRecord &coreOperand = bakedCsg.operands[coreIndex];

    if (canUseCompiledSingleCorePlanePlan(bakedCsg, coreIndex)) {
        if (coreOperand.kind ==
            BakedScene::CsgOperandKind::TransformedQuadric) {
            anyIntersectionFound =
                traceTransformedQuadricCorePlaneIntersection(
                    bakedCsg,
                    bakedCsgs,
                    ray,
                    depthQueue,
                    scratch.getCache(),
                    materialOverride,
                    coreIndex,
                    coreTrueMiss);
        } else {
            java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
                scratch.borrowQueue();
            traceCompiledCoreOperandAllCrossings(
                coreOperand,
                bakedCsgs,
                scratch,
                ray,
                localDepthQueue,
                materialOverride);
            for (IntersectionCandidate &candidate : *localDepthQueue) {
                if (candidateInsideCompiledSingleCorePlaneOperands(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        coreIndex,
                        coreIndex)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
            if (localDepthQueue->size() == 0 &&
                coreOperand.quadricGeometry != nullptr) {
                double d1, d2;
                intersectBakedQuadricWithTrueMiss(
                    *coreOperand.quadricGeometry, ray,
                    ray->getOrigin(), ray->getDirection(),
                    true, scratch.getCache(), coreOperand.quadricViewpointSlot,
                    &d1, &d2, coreTrueMiss);
            }
            scratch.returnQueue(localDepthQueue);
        }

        if (!coreTrueMiss) {
            for (long int p = bakedCsg.planeOperandIndices.size() - 1;
                 p >= 0; p--) {
                const int operandIndex = bakedCsg.planeOperandIndices[p];
                const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[operandIndex];
                IntersectionCandidate candidate;
                if (tracePlaneOperandCandidate(
                        operand,
                        ray,
                        scratch.getCache(),
                        materialOverride,
                        candidate) &&
                    candidateInsideCompiledSingleCorePlaneOperands(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        operandIndex,
                        coreIndex)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
        }

        return anyIntersectionFound;
    }

    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();

    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[i];
        if (i == coreIndex) {
            localDepthQueue->clear();
            tracePlanOperandAllCrossings(
                coreOperand,
                bakedCsgs,
                scratch,
                ray,
                localDepthQueue,
                materialOverride);
            if (localDepthQueue->size() == 0) {
                continue;
            }

            for (IntersectionCandidate &candidate : *localDepthQueue) {
                if (candidateInsideOperandsCoreFirst(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        coreIndex,
                        coreIndex)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
            continue;
        }

        if (operand.isInfinitePlane && operand.nestedCsgProgramIndex < 0) {
            IntersectionCandidate candidate;
            if (tracePlaneOperandCandidate(
                    operand,
                    ray,
                    scratch.getCache(),
                    materialOverride,
                    candidate) &&
                candidateInsideOperandsCoreFirst(
                    bakedCsg,
                    bakedCsgs,
                    candidate.getIntersection().point,
                    i,
                    coreIndex)) {
                depthQueue->offer(candidate);
                anyIntersectionFound = true;
            }
            continue;
        }

        localDepthQueue->clear();
        tracePlanOperandAllCrossings(
            operand,
            bakedCsgs,
            scratch,
            ray,
            localDepthQueue,
            materialOverride);
        if (localDepthQueue->size() == 0) {
            continue;
        }

        for (IntersectionCandidate &candidate : *localDepthQueue) {
            if (candidateInsideAllOtherOperands(
                    bakedCsg,
                    bakedCsgs,
                    candidate.getIntersection().point,
                    i)) {
                depthQueue->offer(candidate);
                anyIntersectionFound = true;
            }
        }
    }

    scratch.returnQueue(localDepthQueue);
    return anyIntersectionFound;
}


int
BakedCsgTrace::traceMorganIntersection(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.planKind ==
            BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
        bakedCsg.specializationValid) {
        return traceSingleCorePlaneIntersection(
            bakedCsg,
            bakedCsgs,
            scratch,
            ray,
            depthQueue,
            materialOverride,
            bakedCsg.specializationCoreOperandIndex);
    }

    return traceMorganIntersectionGeneric(
        bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
}

int
BakedCsgTrace::traceMorganCsg(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.geometryType == BooleanSetOperations::INTERSECTION) {
        return traceMorganIntersection(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }

    if (bakedCsg.geometryType == BooleanSetOperations::UNION &&
        bakedCsg.planKind == BakedScene::CsgPlanKind::GenericMorgan) {
        // Plan 13 Phase 1: only programs with a built cull index (wide
        // unions) pay for the extra branching/gather logic; everything
        // else runs the byte-for-byte pre-Plan-13 function.
        if (bakedCsg.directPrimitiveCullBins != nullptr ||
            bakedCsg.transformedPrimitiveCullBins != nullptr) {
            return traceGenericMorganUnionWithCullBins(
                bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
        }
        return traceGenericMorganUnion(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }

    bool intersectionFound = false;
    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        if (tracePlanOperandAllCrossings(
                bakedCsg.operands[i],
                bakedCsgs,
                scratch,
                ray,
                depthQueue,
                materialOverride)) {
            intersectionFound = true;
        }
    }
    return intersectionFound;
}

bool
BakedCsgTrace::traceAllCrossingsWithScratch(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.algorithm == BakedScene::CsgAlgorithm::RaySegments) {
        return traceRaySegmentCsg(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }
    return traceMorganCsg(
        bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
}

bool
BakedCsgTrace::offerCompiledSingleCorePlaneFirstHitCandidate(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const IntersectionCandidate &candidate,
    long int skipIndex,
    long int coreIndex,
    double &bestT,
    IntersectionCandidate &out)
{
    const double t = candidate.getIntersection().t;
    if (t <= Config::SMALL_TOLERANCE || t >= bestT) {
        return false;
    }
    if (!candidateInsideCompiledSingleCorePlaneOperands(
            bakedCsg,
            bakedCsgs,
            candidate.getIntersection().point,
            skipIndex,
            coreIndex)) {
        return false;
    }
    out = candidate;
    bestT = t;
    return true;
}

bool
BakedCsgTrace::makeTransformedQuadricCandidate(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *ray,
    Material *effectiveMaterial,
    const Vector3Dd &localOrigin,
    const Vector3Dd &localDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
        return false;
    }
    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        operand.objectToLocal.transformPoint(candidate.getIntersection().point);
    const Vector3Dd rayOrigin = ray->getOrigin();
    const Vector3Dd rayDir = ray->getDirection();
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);
    return true;
}

bool
BakedCsgTrace::traceCompiledCoreFirstHitCandidates(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    Material *materialOverride,
    double &bestT,
    IntersectionCandidate &out,
    bool &coreTrueMiss)
{
    coreTrueMiss = false;
    const long int coreIndex = bakedCsg.specializationCoreOperandIndex;
    const BakedScene::CsgOperandRecord &coreOperand = bakedCsg.operands[coreIndex];
    Material *effectiveMaterial =
        coreOperand.material != nullptr ? coreOperand.material : materialOverride;

    if (coreOperand.kind ==
        BakedScene::CsgOperandKind::TransformedQuadric) {
        if (coreOperand.quadricGeometry == nullptr) {
            return false;
        }
        if (coreOperand.bounded && coreOperand.cullSafe &&
            !rayIntersectsAabbForward(*ray, coreOperand.bakedBounds)) {
            return false;
        }

        const Vector3Dd localOrigin =
            coreOperand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            coreOperand.localToObject.transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (!intersectBakedQuadricWithTrueMiss(
                *coreOperand.quadricGeometry,
                ray,
                localOrigin,
                localDirection,
                false,
                scratch.getCache(),
                coreOperand.quadricViewpointSlot,
                &depth1,
                &depth2,
                coreTrueMiss)) {
            return false;
        }

        bool found = false;
        IntersectionCandidate candidate;
        if (makeTransformedQuadricCandidate(
                coreOperand,
                ray,
                effectiveMaterial,
                localOrigin,
                localDirection,
                depth1,
                candidate) &&
            offerCompiledSingleCorePlaneFirstHitCandidate(
                bakedCsg,
                bakedCsgs,
                candidate,
                coreIndex,
                coreIndex,
                bestT,
                out)) {
            found = true;
        }
        if (depth2 != depth1 &&
            makeTransformedQuadricCandidate(
                coreOperand,
                ray,
                effectiveMaterial,
                localOrigin,
                localDirection,
                depth2,
                candidate) &&
            offerCompiledSingleCorePlaneFirstHitCandidate(
                bakedCsg,
                bakedCsgs,
                candidate,
                coreIndex,
                coreIndex,
                bestT,
                out)) {
            found = true;
        }
        return found;
    }

    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();
    bool found = false;
    traceCompiledCoreOperandAllCrossings(
        coreOperand,
        bakedCsgs,
        scratch,
        ray,
        localDepthQueue,
        materialOverride);
    for (IntersectionCandidate &candidate : *localDepthQueue) {
        if (offerCompiledSingleCorePlaneFirstHitCandidate(
                bakedCsg,
                bakedCsgs,
                candidate,
                coreIndex,
                coreIndex,
                bestT,
                out)) {
            found = true;
        }
    }
    if (localDepthQueue->size() == 0 &&
        coreOperand.quadricGeometry != nullptr) {
        double d1, d2;
        intersectBakedQuadricWithTrueMiss(
            *coreOperand.quadricGeometry, ray,
            ray->getOrigin(), ray->getDirection(),
            true, scratch.getCache(), coreOperand.quadricViewpointSlot,
            &d1, &d2, coreTrueMiss);
    }
    scratch.returnQueue(localDepthQueue);
    return found;
}

bool
BakedCsgTrace::traceFirstHitCompiledSingleCorePlane(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    const long int coreIndex = bakedCsg.specializationCoreOperandIndex;
    if (!canUseCompiledSingleCorePlanePlan(bakedCsg, coreIndex)) {
        return false;
    }

    double bestT = Config::MAX_DISTANCE;
    bool coreTrueMiss = false;
    bool found = traceCompiledCoreFirstHitCandidates(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        materialOverride,
        bestT,
        out,
        coreTrueMiss);

    if (!coreTrueMiss) {
        for (long int p = bakedCsg.planeOperandIndices.size() - 1;
             p >= 0; p--) {
            const int operandIndex = bakedCsg.planeOperandIndices[p];
            const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[operandIndex];
            IntersectionCandidate candidate;
            if (tracePlaneOperandCandidate(
                    operand,
                    ray,
                    scratch.getCache(),
                    materialOverride,
                    candidate) &&
                offerCompiledSingleCorePlaneFirstHitCandidate(
                    bakedCsg,
                    bakedCsgs,
                    candidate,
                    operandIndex,
                    coreIndex,
                    bestT,
                    out)) {
                found = true;
            }
        }
    }

    return found;
}

bool
BakedCsgTrace::traceFirstHitByIntersectionMembership(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();
    bool found = false;
    double bestT = Config::MAX_DISTANCE;

    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        localDepthQueue->clear();
        tracePlanOperandAllCrossings(
            bakedCsg.operands[i],
            bakedCsgs,
            scratch,
            ray,
            localDepthQueue,
            materialOverride);

        for (IntersectionCandidate &candidate : *localDepthQueue) {
            const double t = candidate.getIntersection().t;
            if (t <= Config::SMALL_TOLERANCE || t >= bestT) {
                continue;
            }
            if (!candidateInsideAllOtherOperands(
                    bakedCsg,
                    bakedCsgs,
                    candidate.getIntersection().point,
                    i)) {
                continue;
            }
            out = candidate;
            bestT = t;
            found = true;
        }
    }

    scratch.returnQueue(localDepthQueue);
    return found;
}

bool
BakedCsgTrace::traceFirstHitWithScratch(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    if (bakedCsg.operands.size() == 0) {
        return false;
    }

    if (bakedCsg.geometryType == BooleanSetOperations::INTERSECTION) {
        if (bakedCsg.planKind ==
                BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
            bakedCsg.specializationValid) {
            return traceFirstHitCompiledSingleCorePlane(
                bakedCsg,
                bakedCsgs,
                scratch,
                ray,
                out,
                materialOverride);
        }
        return traceFirstHitByIntersectionMembership(
            bakedCsg, bakedCsgs, scratch, ray, out, materialOverride);
    }

    java::PriorityQueue<IntersectionCandidate> *depthQueue =
        scratch.borrowQueue();
    const bool found = traceAllCrossingsWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        depthQueue,
        materialOverride) &&
        depthQueue->size() > 0;
    if (found) {
        out = depthQueue->peek();
    }
    scratch.returnQueue(depthQueue);
    return found;
}


bool
BakedCsgTrace::traceAllCrossings(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride)
{
    CsgScratchContext scratch(ray, &cache);
    const bool found = traceAllCrossingsWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        depthQueue,
        materialOverride);
    scratch.releaseAll();
    return found;
}

bool
BakedCsgTrace::traceFirstHit(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    RaySharedCache &cache,
    Material *materialOverride)
{
    CsgScratchContext scratch(ray, &cache);
    const bool found = traceFirstHitWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        out,
        materialOverride);
    scratch.releaseAll();
    return found;
}

int
BakedCsgTrace::containmentTest(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (bakedCsg.operands.size() == 0) {
        return Geometry::OUTSIDE;
    }

    if (bakedCsg.planKind ==
        BakedScene::CsgPlanKind::DisjointBoundedUnion) {
        for (long int i = 0; i < bakedCsg.operands.size(); i++) {
            const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[i];
            if (!pointInsideAabb(point, operand.bakedBounds, distanceTolerance)) {
                continue;
            }
            if (containmentTestOperand(
                    operand,
                    bakedCsgs,
                    point,
                    distanceTolerance) != Geometry::OUTSIDE) {
                return Geometry::INSIDE;
            }
        }
        return Geometry::OUTSIDE;
    }

    bool isInside;
    switch (bakedCsg.geometryType) {
    case BooleanSetOperations::DIFFERENCE:
        isInside =
            containmentTestOperand(
                bakedCsg.operands[0], bakedCsgs, point, distanceTolerance) != Geometry::OUTSIDE;
        for (long int i = 1; isInside && (i < bakedCsg.operands.size()); i++) {
            if (containmentTestOperand(
                    bakedCsg.operands[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) != Geometry::OUTSIDE) {
                isInside = false;
            }
        }
        break;

    case BooleanSetOperations::INTERSECTION:
        isInside = true;
        for (long int i = 0; isInside && (i < bakedCsg.operands.size()); i++) {
            if (containmentTestOperand(
                    bakedCsg.operands[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) == Geometry::OUTSIDE) {
                isInside = false;
            }
        }
        break;

    default:
        isInside = false;
        for (long int i = 0; !isInside && (i < bakedCsg.operands.size()); i++) {
            if (containmentTestOperand(
                    bakedCsg.operands[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) != Geometry::OUTSIDE) {
                isInside = true;
            }
        }
        break;
    }
    return isInside ? Geometry::INSIDE : Geometry::OUTSIDE;
}

// Explicit instantiation: guarantees a strong, always-emitted definition of
// PriorityQueuePool<IntersectionCandidate>::pop/push. Without this, marking
// those methods `inline` (for header-visible inlining at their many call
// sites) lets the optimizer fully inline every use it sees and elide the
// out-of-line copy in every translation unit, leaving TUs that only forward-
// declare the pool (e.g. BakedCompositeTracing.cpp) with an unresolved symbol.
template java::PriorityQueue<IntersectionCandidate> *
    PriorityQueuePool<IntersectionCandidate>::pop(int);
template void
    PriorityQueuePool<IntersectionCandidate>::push(java::PriorityQueue<IntersectionCandidate> *);
