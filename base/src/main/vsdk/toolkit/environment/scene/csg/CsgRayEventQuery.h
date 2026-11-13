#ifndef VSDK_CSG_RAY_EVENT_QUERY_H
#define VSDK_CSG_RAY_EVENT_QUERY_H
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
class CsgRayLeafBatch;
class CsgRayEventBatch;
// Stable exact-t grouping; nearby values are not asserted coincident. References
// are valid only with the unchanged leaf batch used for this query.
class CsgRayEventQuery {
public:
    static RaySolidQueryStatus group(const CsgRayLeafBatch& leaves, CsgRayEventBatch& result);
};
#endif
