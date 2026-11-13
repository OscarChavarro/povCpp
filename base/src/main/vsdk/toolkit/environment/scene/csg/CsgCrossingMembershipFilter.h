#ifndef VSDK_CSG_CROSSING_MEMBERSHIP_FILTER_H
#define VSDK_CSG_CROSSING_MEMBERSHIP_FILTER_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventReference.h"
class CsgExpression;
class CsgOperandView;
class CsgRayLeafBatch;
class CsgRaySurfaceEvents;
class Ray;
class RayCrossingQuery;
class Vector3Dd;
// The POV-Ray 1.0 CSG heuristic: leaf boundary events are filtered by the
// membership of the event point in sibling operands. A union keeps every
// operand event, so internal surfaces remain. An intersection keeps an event
// of operand i when every other operand contains the point. A difference keeps
// a minuend event when no subtrahend contains it, and a subtrahend event when
// the minuend contains it and no other subtrahend does. Membership is the
// certified classification of the tolerance ball, measured in the coordinates
// of the node whose operands are tested; Boundary counts as contained for an
// intersection operand and for the complement of a subtrahend, as in POV.
// An event is kept only when membership is proven and dropped when one sibling
// proves exclusion; otherwise the query fails with that sibling's status.
class CsgCrossingMembershipFilter {
private:
    static RaySolidQueryStatus select(const CsgExpression& expression, const Ray& ray,
        double tolerance, const CsgRayLeafBatch& leaves, long int& leafIndex,
        java::ArrayList<CsgRayEventReference>& result);
    static RaySolidQueryStatus decide(const CsgExpression& expression, long int operandIndex,
        const Vector3Dd& point, double tolerance, bool& keep);
    static RaySolidQueryStatus toStatus(GeometryContainment containment);
public:
    static RaySolidQueryStatus select(const CsgExpression& expression, const Ray& ray,
        const RayCrossingQuery& query, double tolerance, CsgRaySurfaceEvents& result);
    // Certified world classification of a single operand in its parent space.
    static GeometryContainment classifyOperand(const CsgOperandView& operand,
        const Vector3Dd& point, double tolerance);
};
#endif
