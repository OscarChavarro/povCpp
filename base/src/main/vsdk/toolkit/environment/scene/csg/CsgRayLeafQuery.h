#ifndef VSDK_CSG_RAY_LEAF_QUERY_H
#define VSDK_CSG_RAY_LEAF_QUERY_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
class CsgExpression;
class CsgRayLeafBatch;
class Ray;
class RayCrossingQuery;
// Collects typed leaf intervals, preserving ties; it does not combine them into
// a CSG solid. Failure clears the output and never exposes partial leaves.
class CsgRayLeafQuery {
private:
    static RaySolidQueryStatus collect(const CsgExpression& expression, const Ray& ray,
        const RayCrossingQuery& query, java::ArrayList<const CsgExpression*>& path,
        CsgRayLeafBatch& result);
public:
    static RaySolidQueryStatus query(const CsgExpression& expression, const Ray& ray,
        const RayCrossingQuery& query, CsgRayLeafBatch& result);
};
#endif
