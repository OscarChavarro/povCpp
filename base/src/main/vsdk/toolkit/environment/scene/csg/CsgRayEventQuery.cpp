#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventCursor.h"
RaySolidQueryStatus CsgRayEventQuery::group(const CsgRayLeafBatch& leaves, CsgRayEventBatch& result)
{
    result.clear();
    CsgRayEventBatch candidate;
    java::ArrayList<CsgRayEventCursor> next;
    for (long int i=0;i<leaves.size();++i) {
        const RaySolidClassification& c=leaves.get(i)->getClassification();
        if (c.insideSegments.size()!=c.boundaries.size()+1) return RaySolidQueryStatus::Indeterminate;
        for (long int j=0;j<c.boundaries.size();++j) {
            if (!c.boundaries[j].geometry || !java::Double::isFinite(c.boundaries[j].t) ||
                (j>0 && c.boundaries[j].t<c.boundaries[j-1].t) || c.insideSegments[j]==c.insideSegments[j+1])
                return RaySolidQueryStatus::Indeterminate;
        }
        if (!next.add(CsgRayEventCursor())) return RaySolidQueryStatus::Indeterminate;
    }
    for (;;) {
        bool found=false;
        double t=0;
        for (long int i=0;i<leaves.size();++i) {
            const RaySolidClassification& c=leaves.get(i)->getClassification();
            if (next[i].index<c.boundaries.size() && (!found || c.boundaries[next[i].index].t<t)) {
                t=c.boundaries[next[i].index].t; found=true;
            }
        }
        if (!found) break;
        CsgRayEventGroup group(t);
        for (long int i=0;i<leaves.size();++i) {
            const RaySolidClassification& c=leaves.get(i)->getClassification();
            while (next[i].index<c.boundaries.size() && c.boundaries[next[i].index].t==t) {
                if (!group.add(CsgRayEventReference(i,next[i].index))) return RaySolidQueryStatus::Indeterminate;
                ++next[i].index;
            }
        }
        if (!candidate.add(group)) return RaySolidQueryStatus::Indeterminate;
    }
    result=candidate;
    return RaySolidQueryStatus::Supported;
}
