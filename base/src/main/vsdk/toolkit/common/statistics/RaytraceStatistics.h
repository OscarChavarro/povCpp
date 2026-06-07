#ifndef __VSDK_TOOLKIT_COMMON_STATISTICS_RAYTRACESTATISTICS_H__
#define __VSDK_TOOLKIT_COMMON_STATISTICS_RAYTRACESTATISTICS_H__

class RaytraceStatistics {
public:
    static bool isEnabled();

    static void recordPrimaryRay();
    static void recordShadowRay();
    static void recordReflectionRay();
    static void recordSceneTraversal();
    static void recordObjectIntersectionTest();
    static void recordRayWithT();
    static void recordRayHitInstance();
    static void recordHitInfoClone();
    static void recordGeometryDetailComputation();
    static void printSummary();
};

#endif
