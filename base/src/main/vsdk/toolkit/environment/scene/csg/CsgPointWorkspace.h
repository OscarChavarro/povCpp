#ifndef VSDK_CSG_POINT_WORKSPACE_H
#define VSDK_CSG_POINT_WORKSPACE_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventReference.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayWorkspace.h"
class CsgExpression;
// Caller-owned scratch storage for certified point containment and the POV
// heuristic's event selection. It is reused across queries by one worker and
// must not be shared between concurrent queries; its contents are meaningful
// only during a query. The lists are used as stacks by the recursive queries.
class CsgPointWorkspace {
public:
    java::ArrayList<const CsgExpression*> path;
    java::ArrayList<GeometryContainment> results;
    java::ArrayList<CsgRayEventReference> events;
    // Scratch storage of the leaf ray queries.
    CsgRayWorkspace ray;
    CsgPointWorkspace();
    ~CsgPointWorkspace();
    void clear();
};
#endif
