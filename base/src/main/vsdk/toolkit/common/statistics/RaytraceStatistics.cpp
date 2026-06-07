#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"

bool RaytraceStatistics::isEnabled() { return false; }
void RaytraceStatistics::recordPrimaryRay() {}
void RaytraceStatistics::recordShadowRay() {}
void RaytraceStatistics::recordReflectionRay() {}
void RaytraceStatistics::recordSceneTraversal() {}
void RaytraceStatistics::recordObjectIntersectionTest() {}
void RaytraceStatistics::recordRayWithT() {}
void RaytraceStatistics::recordRayHitInstance() {}
void RaytraceStatistics::recordHitInfoClone() {}
void RaytraceStatistics::recordGeometryDetailComputation() {}
void RaytraceStatistics::printSummary() {}
