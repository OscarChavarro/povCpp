#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRothRayClassifier.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayWorkspace.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"

bool CsgRothRayClassifier::evaluate(const CsgExpression& expression,
    const java::ArrayList<bool>& states, long int& leafIndex)
{
    const CsgOperation operation=expression.getOperation();
    bool result=false;
    for (long int i=0;i<expression.getOperandCount();++i) {
        const CsgOperandView* operand=expression.getOperand(i);
        const CsgExpression* child=operand->getChildExpression();
        // Every operand is visited so that the leaf order stays aligned.
        const bool inside=child ? evaluate(*child,states,leafIndex) : states[leafIndex++];
        result=i==0 ? inside : CsgMembershipRule::combine(operation,result,inside);
    }
    return result;
}

bool CsgRothRayClassifier::isInside(const CsgExpression& expression, const java::ArrayList<bool>& states)
{
    long int leafIndex=0;
    return evaluate(expression,states,leafIndex);
}

RaySolidQueryStatus CsgRothRayClassifier::classify(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, CsgRayClassification& result)
{
    CsgRayWorkspace workspace;
    return classify(expression,ray,query,workspace,result);
}

RaySolidQueryStatus CsgRothRayClassifier::classify(const CsgExpression& expression, const Ray& ray,
    const RayCrossingQuery& query, CsgRayWorkspace& workspace, CsgRayClassification& result)
{
    result.clear();
    RaySolidQueryStatus status=CsgRayLeafQuery::query(expression,ray,query,workspace,result.getLeaves());
    if (status==RaySolidQueryStatus::Supported)
        status=CsgRayEventQuery::validate(result.getLeaves(),workspace.cursors);
    if (status==RaySolidQueryStatus::Supported) status=combine(expression,workspace,result);
    if (status==RaySolidQueryStatus::Supported && !result.getVersions().capture(expression))
        status=RaySolidQueryStatus::Indeterminate;
    if (status!=RaySolidQueryStatus::Supported) result.clear();
    return status;
}

RaySolidQueryStatus CsgRothRayClassifier::combine(const CsgExpression& expression,
    CsgRayWorkspace& workspace, CsgRayClassification& result)
{
    const CsgRayLeafBatch& leaves=result.getLeaves();
    java::ArrayList<bool>& states=workspace.states;
    java::ArrayList<bool>& previous=workspace.previous;
    java::ArrayList<bool>& trial=workspace.trial;
    states.clear(); previous.clear(); trial.clear();
    for (long int i=0;i<leaves.size();++i) {
        const bool initial=leaves.get(i)->getClassification().insideSegments[0];
        if (!states.add(initial) || !previous.add(initial) || !trial.add(initial))
            return RaySolidQueryStatus::Indeterminate;
    }
    RaySolidClassification& solid=result.getSolid();
    bool inside=isInside(expression,states);
    if (!solid.insideSegments.add(inside)) return RaySolidQueryStatus::Indeterminate;
    double t=0;
    java::ArrayList<CsgRayEventReference>& group=workspace.group;
    while (CsgRayEventQuery::nextGroup(leaves,workspace.cursors,t,group)) {
        for (long int i=0;i<states.size();++i) previous.set(i,states[i]);
        for (long int k=0;k<group.size();++k)
            states.set(group[k].leafIndex,
                leaves.get(group[k].leafIndex)->getClassification().insideSegments[group[k].boundaryIndex+1]);
        const bool next=isInside(expression,states);
        if (next==inside) continue;
        // Owners are the leaves whose transition in this group, applied alone
        // to the previous states, already yields the new membership. When no
        // single leaf suffices (e.g. coincident faces of an intersection), the
        // owners are all changed leaves oriented like the combined transition.
        if (!result.beginOwners()) return RaySolidQueryStatus::Indeterminate;
        long int ownerCount=0;
        long int source=0;
        for (int pass=0;pass<2 && ownerCount==0;++pass) {
            for (long int k=0;k<group.size();++k) {
                const long int leaf=group[k].leafIndex;
                if (k+1<group.size() && group[k+1].leafIndex==leaf) continue;
                if (states[leaf]==previous[leaf]) continue;
                if (pass==0) {
                    for (long int i=0;i<previous.size();++i) trial.set(i,previous[i]);
                    trial.set(leaf,states[leaf]);
                    if (isInside(expression,trial)!=next) continue;
                }
                else if ((states[leaf]!=leaves.get(leaf)->getPath().isReversed())!=next) continue;
                if (!result.addOwner(group[k])) return RaySolidQueryStatus::Indeterminate;
                if (ownerCount++==0) source=k;
            }
        }
        const CsgRayEventReference& reference=group[source];
        // Append an empty boundary and assign into it, so the slot's
        // provenance storage is reused instead of copied by value.
        if (!solid.boundaries.add(RayBoundary()) || !solid.insideSegments.add(next))
            return RaySolidQueryStatus::Indeterminate;
        solid.boundaries[solid.boundaries.size()-1]=
            leaves.get(reference.leafIndex)->getClassification().boundaries[reference.boundaryIndex];
        inside=next;
    }
    return RaySolidQueryStatus::Supported;
}
