#ifndef __BAKED_SCENE_STATISTICS__
#define __BAKED_SCENE_STATISTICS__

class BakedSceneStatistics {
  public:
    BakedSceneStatistics(
        const long (&countByKind)[6],
        long csgProgramCount,
        long csgPlanTopLevelPlaneUnion,
        long csgPlanDisjointBoundedUnion,
        long csgPlanSingleCorePlaneIntersection,
        long csgPlanGenericMorgan,
        long csgPlanGenericRaySegments,
        long csgPlanFallback,
        long residualBakedQuadricOperands,
        long residualBakedPlaneOperands,
        long residualTransformedOperands,
        long quadricViewpointSlotCount,
        long planeViewpointSlotCount,
        long residualCategory1NestedCsg,
        long residualCategory1PushdownEligible,
        long residualCategory2EmptySteps,
        long residualCategory3Unbakeable,
        const long (&unionProgramOperandHistogram)[4],
        long unionProgramOperandCullSafeCount,
        long unionProgramOperandTotalCount,
        long topLevelObjectCount,
        long topLevelObjectCullSafeCount) :
        csgProgramCount(csgProgramCount),
        csgPlanTopLevelPlaneUnion(csgPlanTopLevelPlaneUnion),
        csgPlanDisjointBoundedUnion(csgPlanDisjointBoundedUnion),
        csgPlanSingleCorePlaneIntersection(csgPlanSingleCorePlaneIntersection),
        csgPlanGenericMorgan(csgPlanGenericMorgan),
        csgPlanGenericRaySegments(csgPlanGenericRaySegments),
        csgPlanFallback(csgPlanFallback),
        residualBakedQuadricOperands(residualBakedQuadricOperands),
        residualBakedPlaneOperands(residualBakedPlaneOperands),
        residualTransformedOperands(residualTransformedOperands),
        quadricViewpointSlotCount(quadricViewpointSlotCount),
        planeViewpointSlotCount(planeViewpointSlotCount),
        residualCategory1NestedCsg(residualCategory1NestedCsg),
        residualCategory1PushdownEligible(residualCategory1PushdownEligible),
        residualCategory2EmptySteps(residualCategory2EmptySteps),
        residualCategory3Unbakeable(residualCategory3Unbakeable),
        unionProgramOperandCullSafeCount(unionProgramOperandCullSafeCount),
        unionProgramOperandTotalCount(unionProgramOperandTotalCount),
        topLevelObjectCount(topLevelObjectCount),
        topLevelObjectCullSafeCount(topLevelObjectCullSafeCount)
    {
        for (int i = 0; i < 6; i++) {
            this->countByKind[i] = countByKind[i];
        }
        for (int i = 0; i < 4; i++) {
            this->unionProgramOperandHistogram[i] = unionProgramOperandHistogram[i];
        }
    }

    long getCountByKind(int index) const { return countByKind[index]; }
    long getCsgProgramCount() const { return csgProgramCount; }
    long getCsgPlanTopLevelPlaneUnion() const { return csgPlanTopLevelPlaneUnion; }
    long getCsgPlanDisjointBoundedUnion() const { return csgPlanDisjointBoundedUnion; }
    long getCsgPlanSingleCorePlaneIntersection() const { return csgPlanSingleCorePlaneIntersection; }
    long getCsgPlanGenericMorgan() const { return csgPlanGenericMorgan; }
    long getCsgPlanGenericRaySegments() const { return csgPlanGenericRaySegments; }
    long getCsgPlanFallback() const { return csgPlanFallback; }
    long getResidualBakedQuadricOperands() const { return residualBakedQuadricOperands; }
    long getResidualBakedPlaneOperands() const { return residualBakedPlaneOperands; }
    long getResidualTransformedOperands() const { return residualTransformedOperands; }
    long getQuadricViewpointSlotCount() const { return quadricViewpointSlotCount; }
    long getPlaneViewpointSlotCount() const { return planeViewpointSlotCount; }
    long getResidualCategory1NestedCsg() const { return residualCategory1NestedCsg; }
    long getResidualCategory1PushdownEligible() const { return residualCategory1PushdownEligible; }
    long getResidualCategory2EmptySteps() const { return residualCategory2EmptySteps; }
    long getResidualCategory3Unbakeable() const { return residualCategory3Unbakeable; }
    long getUnionProgramOperandHistogram(int index) const { return unionProgramOperandHistogram[index]; }
    long getUnionProgramOperandCullSafeCount() const { return unionProgramOperandCullSafeCount; }
    long getUnionProgramOperandTotalCount() const { return unionProgramOperandTotalCount; }
    long getTopLevelObjectCount() const { return topLevelObjectCount; }
    long getTopLevelObjectCullSafeCount() const { return topLevelObjectCullSafeCount; }

  private:
    long countByKind[6];
    long csgProgramCount;
    long csgPlanTopLevelPlaneUnion;
    long csgPlanDisjointBoundedUnion;
    long csgPlanSingleCorePlaneIntersection;
    long csgPlanGenericMorgan;
    long csgPlanGenericRaySegments;
    long csgPlanFallback;
    long residualBakedQuadricOperands;
    long residualBakedPlaneOperands;
    long residualTransformedOperands;
    long quadricViewpointSlotCount;
    long planeViewpointSlotCount;
    long residualCategory1NestedCsg;
    long residualCategory1PushdownEligible;
    long residualCategory2EmptySteps;
    long residualCategory3Unbakeable;
    long unionProgramOperandHistogram[4];
    long unionProgramOperandCullSafeCount;
    long unionProgramOperandTotalCount;
    long topLevelObjectCount;
    long topLevelObjectCullSafeCount;
};

#endif
