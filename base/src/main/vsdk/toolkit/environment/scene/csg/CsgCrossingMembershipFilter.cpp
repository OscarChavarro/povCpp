#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgCrossingMembershipFilter.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWorkspace.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRaySurfaceEvents.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"

RaySolidQueryStatus CsgCrossingMembershipFilter::toStatus(GeometryContainment containment)
{
    if (containment==GeometryContainment::InvalidInput) return RaySolidQueryStatus::InvalidInput;
    if (containment==GeometryContainment::Unsupported) return RaySolidQueryStatus::Unsupported;
    return RaySolidQueryStatus::Indeterminate;
}

GeometryContainment CsgCrossingMembershipFilter::classifyOperand(const CsgOperandView& operand,
    const Vector3Dd& point, double tolerance)
{
    CsgPointWorkspace workspace;
    return CsgPointClassification::classifyOperandContainment(operand,point,tolerance,workspace);
}

RaySolidQueryStatus CsgCrossingMembershipFilter::select(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, double tolerance, CsgRaySurfaceEvents& result)
{
    CsgPointWorkspace workspace;
    return select(expression,ray,query,tolerance,workspace,result);
}

RaySolidQueryStatus CsgCrossingMembershipFilter::select(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, double tolerance, CsgPointWorkspace& workspace,
    CsgRaySurfaceEvents& result)
{
    result.clear();
    workspace.clear();
    if (!java::Double::isFinite(tolerance) || tolerance<0) return RaySolidQueryStatus::InvalidInput;
    RaySolidQueryStatus status=CsgRayLeafQuery::query(expression,ray,query,workspace.ray,result.getLeaves());
    long int leafIndex=0;
    if (status==RaySolidQueryStatus::Supported)
        status=select(expression,ray,tolerance,result.getLeaves(),leafIndex,workspace);
    for (long int i=0;status==RaySolidQueryStatus::Supported && i<workspace.events.size();++i)
        if (!result.add(workspace.events[i])) status=RaySolidQueryStatus::Indeterminate;
    if (status==RaySolidQueryStatus::Supported && !result.getVersions().capture(expression))
        status=RaySolidQueryStatus::Indeterminate;
    if (status!=RaySolidQueryStatus::Supported) result.clear();
    return status;
}

RaySolidQueryStatus CsgCrossingMembershipFilter::decide(const CsgExpression& expression,
    long int operandIndex, const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace,
    bool& keep)
{
    const CsgOperation operation=expression.getOperation();
    bool excluded=false;
    RaySolidQueryStatus uncertain=RaySolidQueryStatus::Supported;
    for (long int j=0;j<expression.getOperandCount() && !excluded;++j) {
        if (j==operandIndex) continue;
        const bool mustContain=CsgMembershipRule::requiresContainment(operation,operandIndex,j);
        const CsgOperandView* sibling=expression.getOperand(j);
        const GeometryContainment c=sibling ?
            CsgPointClassification::classifyOperandContainment(*sibling,point,tolerance,workspace) :
            GeometryContainment::InvalidInput;
        if (c==GeometryContainment::InvalidInput) return RaySolidQueryStatus::InvalidInput;
        if (c==GeometryContainment::Boundary) continue;
        if (c==GeometryContainment::Inside) { excluded=!mustContain; continue; }
        if (c==GeometryContainment::Outside) { excluded=mustContain; continue; }
        if (uncertain==RaySolidQueryStatus::Supported) uncertain=toStatus(c);
    }
    keep=false;
    if (excluded) return RaySolidQueryStatus::Supported;
    if (uncertain!=RaySolidQueryStatus::Supported) return uncertain;
    keep=true;
    return RaySolidQueryStatus::Supported;
}

RaySolidQueryStatus CsgCrossingMembershipFilter::select(const CsgExpression& expression, const Ray& ray,
    double tolerance, const CsgRayLeafBatch& leaves, long int& leafIndex,
    CsgPointWorkspace& workspace)
{
    const CsgOperation operation=expression.getOperation();
    java::ArrayList<CsgRayEventReference>& events=workspace.events;
    for (long int i=0;i<expression.getOperandCount();++i) {
        const CsgOperandView* operand=expression.getOperand(i);
        const CsgExpression* child=operand->getChildExpression();
        // This operand's events occupy the stack from `first` on.
        const long int first=events.size();
        if (child) {
            // The leaf query has already validated this placement.
            Matrix4x4d forward,inverse;
            if (operand->queryAffinePlacement(forward,inverse)!=AffinePlacementStatus::Valid)
                return RaySolidQueryStatus::Indeterminate;
            const Ray childRay=Ray::fromAffine(inverse.multiply(ray.getOrigin()),
                inverse.withoutTranslation().multiply(ray.getDirection()),ray.getT());
            const RaySolidQueryStatus status=select(*child,childRay,tolerance,leaves,leafIndex,workspace);
            if (status!=RaySolidQueryStatus::Supported) return status;
        }
        else {
            const CsgRayLeaf* leaf=leaves.get(leafIndex);
            if (!leaf) return RaySolidQueryStatus::Indeterminate;
            for (long int k=0;k<leaf->getClassification().boundaries.size();++k)
                if (!events.add(CsgRayEventReference(leafIndex,k))) return RaySolidQueryStatus::Indeterminate;
            ++leafIndex;
        }
        if (!CsgMembershipRule::filtersEvents(operation)) continue;
        // Keep the selected events in order, compacting them in place.
        long int kept=first;
        const long int last=events.size();
        for (long int k=first;k<last;++k) {
            bool keep=true;
            const CsgRayEventReference event=events[k];
            const double t=leaves.get(event.leafIndex)->getClassification().boundaries[event.boundaryIndex].t;
            const Vector3Dd point=ray.getOrigin().add(ray.getDirection().multiply(t));
            const RaySolidQueryStatus status=decide(expression,i,point,tolerance,workspace,keep);
            if (status!=RaySolidQueryStatus::Supported) return status;
            if (keep) events[kept++]=event;
        }
        while (events.size()>kept) events.remove(events.size()-1);
    }
    return RaySolidQueryStatus::Supported;
}
