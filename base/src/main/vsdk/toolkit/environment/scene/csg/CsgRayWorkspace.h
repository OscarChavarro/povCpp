#ifndef VSDK_CSG_RAY_WORKSPACE_H
#define VSDK_CSG_RAY_WORKSPACE_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventReference.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventCursor.h"
class CsgExpression;
// Caller-owned scratch storage for CSG ray queries. It is reused across
// queries by one worker and must not be shared between concurrent queries.
// Its contents are meaningful only during a query.
class CsgRayWorkspace {
public:
    java::ArrayList<const CsgExpression*> path;
    java::ArrayList<CsgRayEventCursor> cursors;
    java::ArrayList<CsgRayEventReference> group;
    java::ArrayList<bool> states;
    java::ArrayList<bool> previous;
    java::ArrayList<bool> trial;
    CsgRayWorkspace();
    ~CsgRayWorkspace();
};
#endif
