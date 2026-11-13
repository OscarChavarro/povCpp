#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgCrossingMembershipFilter.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
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
    CsgTree single(CsgOperation::UNION);
    if (!single.addOperand(operand)) return GeometryContainment::InvalidInput;
    return CsgPointClassification::classify(single,point,tolerance);
}

RaySolidQueryStatus CsgCrossingMembershipFilter::select(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, double tolerance, CsgRaySurfaceEvents& result)
{
    result.clear();
    if (!java::Double::isFinite(tolerance) || tolerance<0) return RaySolidQueryStatus::InvalidInput;
    RaySolidQueryStatus status=CsgRayLeafQuery::query(expression,ray,query,result.getLeaves());
    java::ArrayList<CsgRayEventReference> selected(8);
    long int leafIndex=0;
    if (status==RaySolidQueryStatus::Supported)
        status=select(expression,ray,tolerance,result.getLeaves(),leafIndex,selected);
    for (long int i=0;status==RaySolidQueryStatus::Supported && i<selected.size();++i)
        if (!result.add(selected[i])) status=RaySolidQueryStatus::Indeterminate;
    if (status==RaySolidQueryStatus::Supported && !result.getVersions().capture(expression))
        status=RaySolidQueryStatus::Indeterminate;
    if (status!=RaySolidQueryStatus::Supported) result.clear();
    return status;
}

RaySolidQueryStatus CsgCrossingMembershipFilter::decide(const CsgExpression& expression,
    long int operandIndex, const Vector3Dd& point, double tolerance, bool& keep)
{
    const CsgOperation operation=expression.getOperation();
    bool excluded=false;
    RaySolidQueryStatus uncertain=RaySolidQueryStatus::Supported;
    for (long int j=0;j<expression.getOperandCount() && !excluded;++j) {
        if (j==operandIndex) continue;
        const bool mustContain=CsgMembershipRule::requiresContainment(operation,operandIndex,j);
        const GeometryContainment c=classifyOperand(*expression.getOperand(j),point,tolerance);
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
    java::ArrayList<CsgRayEventReference>& result)
{
    const CsgOperation operation=expression.getOperation();
    for (long int i=0;i<expression.getOperandCount();++i) {
        const CsgOperandView* operand=expression.getOperand(i);
        const CsgExpression* child=operand->getChildExpression();
        java::ArrayList<CsgRayEventReference> events(8);
        if (child) {
            // The leaf query has already validated this placement.
            Matrix4x4d forward,inverse;
            if (operand->queryAffinePlacement(forward,inverse)!=AffinePlacementStatus::Valid)
                return RaySolidQueryStatus::Indeterminate;
            const Ray childRay=Ray::fromAffine(inverse.multiply(ray.getOrigin()),
                inverse.withoutTranslation().multiply(ray.getDirection()),ray.getT());
            const RaySolidQueryStatus status=select(*child,childRay,tolerance,leaves,leafIndex,events);
            if (status!=RaySolidQueryStatus::Supported) return status;
        }
        else {
            const CsgRayLeaf* leaf=leaves.get(leafIndex);
            if (!leaf) return RaySolidQueryStatus::Indeterminate;
            for (long int k=0;k<leaf->getClassification().boundaries.size();++k)
                if (!events.add(CsgRayEventReference(leafIndex,k))) return RaySolidQueryStatus::Indeterminate;
            ++leafIndex;
        }
        for (long int k=0;k<events.size();++k) {
            bool keep=true;
            if (CsgMembershipRule::filtersEvents(operation)) {
                const double t=leaves.get(events[k].leafIndex)->getClassification().boundaries[events[k].boundaryIndex].t;
                const Vector3Dd point=ray.getOrigin().add(ray.getDirection().multiply(t));
                const RaySolidQueryStatus status=decide(expression,i,point,tolerance,keep);
                if (status!=RaySolidQueryStatus::Supported) return status;
            }
            if (keep && !result.add(events[k])) return RaySolidQueryStatus::Indeterminate;
        }
    }
    return RaySolidQueryStatus::Supported;
}
