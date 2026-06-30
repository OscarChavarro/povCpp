#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "common/Config.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/geometry/volume/constructiveSolidGeometry/RaySegments.h"
#include "render/BakedCsgTracing.h"

namespace {
struct CsgScratchContext {
    static constexpr int MAX_SCRATCH_QUEUES = 8;

    java::PriorityQueue<IntersectionCandidate> *queues[MAX_SCRATCH_QUEUES] = {};
    int used = 0;
    RayWithSegments *ownerRay = nullptr;

    java::PriorityQueue<IntersectionCandidate> *borrowQueue()
    {
        if (used >= MAX_SCRATCH_QUEUES) {
            return ownerRay->getIntersectionQueuePool()->pop(128);
        }
        if (queues[used] == nullptr) {
            queues[used] = ownerRay->getIntersectionQueuePool()->pop(128);
        }
        java::PriorityQueue<IntersectionCandidate> *queue = queues[used++];
        queue->clear();
        return queue;
    }

    void returnQueue(java::PriorityQueue<IntersectionCandidate> *queue)
    {
        queue->clear();
        if (used > 0 && queues[used - 1] == queue) {
            used--;
            return;
        }
        ownerRay->getIntersectionQueuePool()->push(queue);
    }
};

bool traceAllCrossingsWithScratch(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride);

bool traceFirstHitWithScratch(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride);

bool
rayIntersectsAabbForward(const RayWithSegments &ray, const AxisAlignedBox &box)
{
    const Vector3Dd origin = ray.getOrigin();
    const Vector3Dd direction = ray.getDirection();
    double tMin = 0.0;
    double tMax = 1e30;

    auto updateAxis = [&](double originCoord, double directionCoord,
                          double minCoord, double maxCoord) -> bool {
        if (directionCoord > -1e-12 && directionCoord < 1e-12) {
            return originCoord >= minCoord && originCoord <= maxCoord;
        }
        const double invDir = 1.0 / directionCoord;
        double nearT = (minCoord - originCoord) * invDir;
        double farT = (maxCoord - originCoord) * invDir;
        if (nearT > farT) {
            const double tmp = nearT;
            nearT = farT;
            farT = tmp;
        }
        tMin = nearT > tMin ? nearT : tMin;
        tMax = farT < tMax ? farT : tMax;
        return tMin <= tMax;
    };

    return
        updateAxis(origin.x(), direction.x(), box.min.x(), box.max.x()) &&
        updateAxis(origin.y(), direction.y(), box.min.y(), box.max.y()) &&
        updateAxis(origin.z(), direction.z(), box.min.z(), box.max.z()) &&
        tMax >= 0.0;
}

bool
pointInsideAabb(const Vector3Dd &point, const AxisAlignedBox &box, double tolerance)
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
annotateDirectCandidates(
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const Scene::BakedCsgOperand &operand)
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

bool
traceOperandAllCrossings(
    const Scene::BakedCsgOperand &operand,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
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
    if (!operand.hasTransform && operand.bakedCsgIndex < 0 &&
        !operand.isInfinitePlane) {
        if (operand.geometry->hasNativeAnnotatedCrossings()) {
            GeometryIntersectionEmissionContext context;
            context.materialOverride = effectiveMaterial;
            context.detailOwner = operand.operand;
            context.materialUsesObjectLocalPoint = true;
            return operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
                ray, depthQueue, context);
        }

        const int initialSize = depthQueue->size();
        const bool found = operand.geometry->doIntersectionForAllRayCrossings(
            ray, depthQueue, effectiveMaterial);
        if (!found || depthQueue->size() == initialSize) {
            return false;
        }
        return annotateDirectCandidates(depthQueue, operand);
    }

    RayWithSegments *localRayPtr = ray;
    if (operand.hasTransform) {
        RayWithSegments localRay(RayWithSegments::LocalIntersectionClone{}, *ray);
        localRay.setOrigin(operand.localToObject.transformPoint(ray->getOrigin()));
        localRay.setDirection(operand.localToObject.transformDirection(ray->getDirection()));
        localRay.setQuadricConstantsCached(false);
        localRayPtr = &localRay;

        if (operand.isInfinitePlane && operand.bakedCsgIndex < 0) {
            double depth;
            if (!InfinitePlane::intersectPlane(
                    localRayPtr,
                    static_cast<InfinitePlane *>(operand.geometry),
                    &depth) ||
                depth <= Config::SMALL_TOLERANCE) {
                return false;
            }

            IntersectionCandidate candidate;
            candidate.getIntersection().point =
                localRayPtr->getOrigin().add(localRayPtr->getDirection().multiply(depth));
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
            depthQueue->offer(candidate);
            return true;
        }

        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            scratch.borrowQueue();

        bool found = false;
        if (operand.bakedCsgIndex >= 0) {
            found = traceAllCrossingsWithScratch(
                bakedCsgs[operand.bakedCsgIndex],
                bakedCsgs,
                scratch,
                localRayPtr,
                localDepthQueue,
                effectiveMaterial);
        } else {
            found = operand.geometry->doIntersectionForAllRayCrossings(
                localRayPtr, localDepthQueue, effectiveMaterial);
        }

        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        const double dirLenSq = rayDir.dotProduct(rayDir);
        for (IntersectionCandidate &candidate : *localDepthQueue) {
            candidate.getAttributes().pushDetailOwner(operand.operand);
            candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
            candidate.getIntersection().point =
                operand.objectToLocal.transformPoint(candidate.getIntersection().point);
            candidate.getIntersection().t =
                candidate.getIntersection().point
                    .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
            depthQueue->offer(candidate);
        }

        scratch.returnQueue(localDepthQueue);
        return found;
    }

    if (operand.isInfinitePlane && operand.bakedCsgIndex < 0) {
        double depth;
        if (!InfinitePlane::intersectPlane(
                ray,
                static_cast<InfinitePlane *>(operand.geometry),
                &depth) ||
            depth <= Config::SMALL_TOLERANCE) {
            return false;
        }

        IntersectionCandidate candidate;
        candidate.getIntersection().point =
            ray->getOrigin().add(ray->getDirection().multiply(depth));
        candidate.getAttributes().setHitGeometry(operand.geometry);
        candidate.getAttributes().setMaterial(effectiveMaterial);
        candidate.getAttributes().pushDetailOwner(operand.operand);
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
        candidate.getIntersection().t = depth;
        depthQueue->offer(candidate);
        return true;
    }

    const Vector3Dd rayOrigin = ray->getOrigin();
    const Vector3Dd rayDir = ray->getDirection();
    const double dirLenSq = rayDir.dotProduct(rayDir);
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();
    bool found = false;
    if (operand.bakedCsgIndex >= 0) {
        found = traceAllCrossingsWithScratch(
            bakedCsgs[operand.bakedCsgIndex],
            bakedCsgs,
            scratch,
            ray,
            localDepthQueue,
            effectiveMaterial);
    } else {
        found = operand.geometry->doIntersectionForAllRayCrossings(
            ray, localDepthQueue, effectiveMaterial);
    }
    for (IntersectionCandidate &candidate : *localDepthQueue) {
        candidate.getAttributes().pushDetailOwner(operand.operand);
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
        candidate.getIntersection().t =
            candidate.getIntersection().point
                .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
        depthQueue->offer(candidate);
    }

    scratch.returnQueue(localDepthQueue);
    return found;
}

int
containmentTestOperand(
    const Scene::BakedCsgOperand &operand,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (operand.geometry == nullptr) {
        return Geometry::OUTSIDE;
    }
    const Vector3Dd localPoint = operand.hasTransform ?
        operand.localToObject.transformPoint(point) : point;
    if (operand.bakedCsgIndex >= 0) {
        return BakedCsgTracing::containmentTest(
            bakedCsgs[operand.bakedCsgIndex],
            bakedCsgs,
            localPoint,
            distanceTolerance);
    }
    return operand.geometry->doContainmentTest(localPoint, distanceTolerance);
}

RaySegments
buildRaySegments(
    const Scene::BakedCsgOperand &operand,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();

    traceOperandAllCrossings(
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

static bool combineUnion(bool insideLeft, bool insideRight)
{
    return insideLeft || insideRight;
}

static bool combineIntersection(bool insideLeft, bool insideRight)
{
    return insideLeft && insideRight;
}

static bool combineDifference(bool insideLeft, bool insideRight)
{
    return insideLeft && !insideRight;
}

RaySegments
mergeByMembership(
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
traceRaySegmentCsg(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.operands.size() == 0) {
        return false;
    }

    if (bakedCsg.specialization ==
            Scene::BakedCsgSpecialization::TopLevelPlaneUnion ||
        bakedCsg.specialization ==
            Scene::BakedCsgSpecialization::DisjointBoundedUnion) {
        bool anyFound = false;
        for (long int i = 0; i < bakedCsg.operands.size(); i++) {
            if (traceOperandAllCrossings(
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
candidateInsideAllOtherOperands(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
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
tracePlaneOperandCandidate(
    const Scene::BakedCsgOperand &operand,
    RayWithSegments *ray,
    Material *materialOverride,
    IntersectionCandidate &candidate)
{
    if (!operand.isInfinitePlane || operand.bakedCsgIndex >= 0 ||
        operand.geometry == nullptr) {
        return false;
    }

    Material *effectiveMaterial =
        operand.material != nullptr ? operand.material : materialOverride;
    if (operand.hasTransform) {
        RayWithSegments localRay(RayWithSegments::LocalIntersectionClone{}, *ray);
        localRay.setOrigin(operand.localToObject.transformPoint(ray->getOrigin()));
        localRay.setDirection(operand.localToObject.transformDirection(ray->getDirection()));
        localRay.setQuadricConstantsCached(false);

        double depth;
        if (!InfinitePlane::intersectPlane(
                &localRay,
                static_cast<InfinitePlane *>(operand.geometry),
                &depth) ||
            depth <= Config::SMALL_TOLERANCE) {
            return false;
        }

        candidate = IntersectionCandidate();
        candidate.getIntersection().point =
            localRay.getOrigin().add(localRay.getDirection().multiply(depth));
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
    if (!InfinitePlane::intersectPlane(
            ray,
            static_cast<InfinitePlane *>(operand.geometry),
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

int
traceSingleCorePlaneIntersection(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    long int coreIndex)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();
    bool anyIntersectionFound = false;

    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        const Scene::BakedCsgOperand &operand = bakedCsg.operands[i];
        if (operand.isInfinitePlane && operand.bakedCsgIndex < 0) {
            IntersectionCandidate candidate;
            if (tracePlaneOperandCandidate(
                    operand,
                    ray,
                    materialOverride,
                    candidate) &&
                candidateInsideAllOtherOperands(
                    bakedCsg,
                    bakedCsgs,
                    candidate.getIntersection().point,
                    i)) {
                depthQueue->offer(candidate);
                anyIntersectionFound = true;
            }
            continue;
        }

        localDepthQueue->clear();
        traceOperandAllCrossings(
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

    (void)coreIndex;
    scratch.returnQueue(localDepthQueue);
    return anyIntersectionFound;
}

int
traceMorganIntersection(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.specialization ==
        Scene::BakedCsgSpecialization::SingleCorePlaneIntersection) {
        return traceSingleCorePlaneIntersection(
            bakedCsg,
            bakedCsgs,
            scratch,
            ray,
            depthQueue,
            materialOverride,
            bakedCsg.specializationCoreOperandIndex);
    }

    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();
    bool anyIntersectionFound = false;

    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        const Scene::BakedCsgOperand &localShape = bakedCsg.operands[i];
        traceOperandAllCrossings(
            localShape, bakedCsgs, scratch, ray, localDepthQueue, materialOverride);

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
        localDepthQueue->clear();
    }

    scratch.returnQueue(localDepthQueue);
    return anyIntersectionFound;
}

int
traceMorganCsg(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.geometryType == BooleanSetOperations::INTERSECTION) {
        return traceMorganIntersection(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }

    bool intersectionFound = false;
    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        if (traceOperandAllCrossings(
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
traceAllCrossingsWithScratch(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (bakedCsg.algorithm == Scene::BakedCsgAlgorithm::RaySegments) {
        return traceRaySegmentCsg(
            bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
    }
    return traceMorganCsg(
        bakedCsg, bakedCsgs, scratch, ray, depthQueue, materialOverride);
}

bool
traceFirstHitByIntersectionMembership(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
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
        traceOperandAllCrossings(
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
traceFirstHitWithScratch(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    if (bakedCsg.operands.size() == 0) {
        return false;
    }

    if (bakedCsg.geometryType == BooleanSetOperations::INTERSECTION) {
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
}

bool
BakedCsgTracing::traceAllCrossings(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    CsgScratchContext scratch;
    scratch.ownerRay = ray;
    const bool found = traceAllCrossingsWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        depthQueue,
        materialOverride);
    for (int i = CsgScratchContext::MAX_SCRATCH_QUEUES - 1; i >= 0; i--) {
        if (scratch.queues[i] != nullptr) {
            ray->getIntersectionQueuePool()->push(scratch.queues[i]);
        }
    }
    return found;
}

bool
BakedCsgTracing::traceFirstHit(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    CsgScratchContext scratch;
    scratch.ownerRay = ray;
    const bool found = traceFirstHitWithScratch(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        out,
        materialOverride);
    for (int i = CsgScratchContext::MAX_SCRATCH_QUEUES - 1; i >= 0; i--) {
        if (scratch.queues[i] != nullptr) {
            ray->getIntersectionQueuePool()->push(scratch.queues[i]);
        }
    }
    return found;
}

int
BakedCsgTracing::containmentTest(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (bakedCsg.operands.size() == 0) {
        return Geometry::OUTSIDE;
    }

    if (bakedCsg.specialization ==
        Scene::BakedCsgSpecialization::DisjointBoundedUnion) {
        for (long int i = 0; i < bakedCsg.operands.size(); i++) {
            const Scene::BakedCsgOperand &operand = bakedCsg.operands[i];
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
