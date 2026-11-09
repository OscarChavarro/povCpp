#ifndef __RAYTRACE_STATISTICS__
#define __RAYTRACE_STATISTICS__

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
