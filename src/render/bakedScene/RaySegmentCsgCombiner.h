#ifndef __RAY_SEGMENT_CSG_COMBINER__
#define __RAY_SEGMENT_CSG_COMBINER__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/geometry/volume/constructiveSolidGeometry/RaySegments.h"
#include "environment/material/Material.h"
#include "java/util/ArrayList.h"
#include "java/util/PriorityQueue.h"
#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CsgScratchContext.h"

// Boolean combination of baked CSG operands via ray segments: builds each
// operand's inside/outside membership intervals along the ray, then merges
// pairs of operand segment lists according to the CSG's boolean operator
// (union/intersection/difference). Used for programs that did not qualify
// for one of the compiled fast paths (SingleCorePlaneCsgTrace,
// CsgMorganUnionTrace).
class RaySegmentCsgCombiner {
public:
    static int traceRaySegmentCsg(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

private:
    static RaySegments buildRaySegments(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        Material *materialOverride);

    static bool combineUnion(bool insideLeft, bool insideRight);
    static bool combineIntersection(bool insideLeft, bool insideRight);
    static bool combineDifference(bool insideLeft, bool insideRight);

    static RaySegments mergeByMembership(
        const RaySegments &left,
        const RaySegments &right,
        bool (*combine)(bool insideLeft, bool insideRight));
};

#endif
