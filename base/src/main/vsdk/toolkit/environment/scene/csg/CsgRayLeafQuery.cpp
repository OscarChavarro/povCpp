#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayWorkspace.h"

RaySolidQueryStatus CsgRayLeafQuery::query(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, CsgRayLeafBatch& result)
{
    CsgRayWorkspace workspace;
    return CsgRayLeafQuery::query(expression,ray,query,workspace,result);
}

RaySolidQueryStatus CsgRayLeafQuery::query(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, CsgRayWorkspace& workspace, CsgRayLeafBatch& result)
{
    result.clear();
    const Vector3Dd& origin=ray.getOrigin();
    const Vector3Dd& direction=ray.getDirection();
    if (!java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) ||
        !java::Double::isFinite(origin.z()) || !java::Double::isFinite(direction.x()) ||
        !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()) ||
        (direction.x()==0 && direction.y()==0 && direction.z()==0) ||
        java::Double::isNaN(query.minT) || java::Double::isNaN(query.maxT) || query.minT>=query.maxT ||
        query.minimumChordSquared!=0 || query.minimumExteriorApproach!=java::Double::NEGATIVE_INFINITY)
        return RaySolidQueryStatus::InvalidInput;
    workspace.path.clear();
    const RaySolidQueryStatus status=collect(expression,ray,query,workspace.path,result);
    if (status!=RaySolidQueryStatus::Supported) result.clear();
    return status;
}

RaySolidQueryStatus CsgRayLeafQuery::collect(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, java::ArrayList<const CsgExpression*>& path, CsgRayLeafBatch& result)
{
    if (path.contains(&expression)) return RaySolidQueryStatus::InvalidInput;
    const CsgOperation operation=expression.getOperation();
    const long int count=expression.getOperandCount();
    if (count<=0 || (operation!=CsgOperation::UNION && operation!=CsgOperation::INTERSECTION &&
        operation!=CsgOperation::DIFFERENCE)) return RaySolidQueryStatus::InvalidInput;
    if (!path.add(&expression)) return RaySolidQueryStatus::Indeterminate;
    for (long int i=0;i<count;++i) {
        const CsgOperandView* operand=expression.getOperand(i);
        if (!operand) { path.remove(path.size()-1); return RaySolidQueryStatus::InvalidInput; }
        const long int first=result.size();
        const CsgExpression* child=operand->getChildExpression();
        Matrix4x4d forward,inverse;
        RaySolidQueryStatus status=RaySolidQueryStatus::Supported;
        if (child && (!operand->isStructuralTraversalSafe() || !child->isStructuralTraversalSafe())) status=RaySolidQueryStatus::Unsupported;
        else if (child) {
            const AffinePlacementStatus placement=operand->queryAffinePlacement(forward,inverse);
            if (placement==AffinePlacementStatus::Invalid) status=RaySolidQueryStatus::InvalidInput;
            else if (placement==AffinePlacementStatus::Unsupported) status=RaySolidQueryStatus::Unsupported;
            else if (placement!=AffinePlacementStatus::Valid) status=RaySolidQueryStatus::Indeterminate;
            else {
                const Vector3Dd origin=inverse.multiply(ray.getOrigin());
                const Vector3Dd direction=inverse.withoutTranslation().multiply(ray.getDirection());
                if (!java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) ||
                    !java::Double::isFinite(origin.z()) || !java::Double::isFinite(direction.x()) ||
                    !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()) ||
                    (direction.x()==0 && direction.y()==0 && direction.z()==0))
                    status=RaySolidQueryStatus::Indeterminate;
                else status=collect(*child,Ray::fromAffine(origin,direction,ray.getT()),query,path,result);
            }
        }
        else {
            CsgRayLeaf* leaf=result.acquire();
            if (!leaf) status=RaySolidQueryStatus::Indeterminate;
            else status=operand->classifyRay(ray,query,leaf->getClassification());
            if (status==RaySolidQueryStatus::Supported) {
                const RaySolidClassification& classification=leaf->getClassification();
                if (classification.insideSegments.size()!=classification.boundaries.size()+1)
                    status=RaySolidQueryStatus::Indeterminate;
                for (long int k=0;status==RaySolidQueryStatus::Supported && k<classification.boundaries.size();++k) {
                    const double t=classification.boundaries[k].t;
                    if (!classification.boundaries[k].geometry || !java::Double::isFinite(t) ||
                        t<query.minT || t>query.maxT || (k>0 && t<classification.boundaries[k-1].t) ||
                        classification.insideSegments[k]==classification.insideSegments[k+1])
                        status=RaySolidQueryStatus::Indeterminate;
                }
            }
        }
        if (status!=RaySolidQueryStatus::Supported) { path.remove(path.size()-1); return status; }
        for (long int j=first;j<result.size();++j) {
            CsgRayLeaf* leaf=result.get(j);
            if (!leaf->prependOperand(operand) || (child && !leaf->appendPlacement(operand,forward,inverse))) {
                path.remove(path.size()-1); return RaySolidQueryStatus::Indeterminate;
            }
            if (operation==CsgOperation::DIFFERENCE && i>0) leaf->reverseOrientation();
        }
    }
    path.remove(path.size()-1);
    return RaySolidQueryStatus::Supported;
}
