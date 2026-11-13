#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventCursor.h"

RaySolidQueryStatus CsgRayEventQuery::validate(const CsgRayLeafBatch& leaves, java::ArrayList<CsgRayEventCursor>& cursors)
{
    cursors.clear();
    for (long int i=0;i<leaves.size();++i) {
        const RaySolidClassification& c=leaves.get(i)->getClassification();
        if (c.insideSegments.size()!=c.boundaries.size()+1) return RaySolidQueryStatus::Indeterminate;
        for (long int j=0;j<c.boundaries.size();++j) {
            if (!c.boundaries[j].geometry || !java::Double::isFinite(c.boundaries[j].t) ||
                (j>0 && c.boundaries[j].t<c.boundaries[j-1].t) || c.insideSegments[j]==c.insideSegments[j+1])
                return RaySolidQueryStatus::Indeterminate;
        }
        if (!cursors.add(CsgRayEventCursor())) return RaySolidQueryStatus::Indeterminate;
    }
    return RaySolidQueryStatus::Supported;
}

bool CsgRayEventQuery::nextGroup(const CsgRayLeafBatch& leaves, java::ArrayList<CsgRayEventCursor>& cursors,
    double& t, java::ArrayList<CsgRayEventReference>& group)
{
    group.clear();
    bool found=false;
    for (long int i=0;i<leaves.size() && i<cursors.size();++i) {
        const RaySolidClassification& c=leaves.get(i)->getClassification();
        if (cursors[i].index<c.boundaries.size() && (!found || c.boundaries[cursors[i].index].t<t)) {
            t=c.boundaries[cursors[i].index].t; found=true;
        }
    }
    if (!found) return false;
    for (long int i=0;i<leaves.size() && i<cursors.size();++i) {
        const RaySolidClassification& c=leaves.get(i)->getClassification();
        while (cursors[i].index<c.boundaries.size() && c.boundaries[cursors[i].index].t==t) {
            if (!group.add(CsgRayEventReference(i,cursors[i].index))) { group.clear(); return false; }
            ++cursors[i].index;
        }
    }
    return true;
}

RaySolidQueryStatus CsgRayEventQuery::group(const CsgRayLeafBatch& leaves, CsgRayEventBatch& result)
{
    result.clear();
    java::ArrayList<CsgRayEventCursor> cursors(leaves.size()>0 ? leaves.size() : 1);
    const RaySolidQueryStatus status=validate(leaves,cursors);
    if (status!=RaySolidQueryStatus::Supported) return status;
    CsgRayEventBatch candidate;
    java::ArrayList<CsgRayEventReference> references(8);
    double t=0;
    while (nextGroup(leaves,cursors,t,references)) {
        CsgRayEventGroup group(t);
        for (long int k=0;k<references.size();++k)
            if (!group.add(references[k])) return RaySolidQueryStatus::Indeterminate;
        if (!candidate.add(group)) return RaySolidQueryStatus::Indeterminate;
    }
    result=candidate;
    return RaySolidQueryStatus::Supported;
}
