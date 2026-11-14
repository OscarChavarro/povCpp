#ifndef VSDK_CSG_SEGMENT_CLASSIFICATION_H
#define VSDK_CSG_SEGMENT_CLASSIFICATION_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperation.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSegmentEvent.h"

/**
The fast [ROTH1982] section 3.3 ray classification over the caller's event
type: the state at the start of the ray and the sorted events where it
alternates. A leaf classification is built from one containment sample and
the leaf's sorted crossings (`append` toggles the state); `merge` combines two
classifications by `CsgMembershipRule::combine` (Table 3), keeping only the
events where the combined state changes. Ties take the left event first.
No tolerance, certificate or provenance is involved: renderers use it when
their own crossings and containment are trusted; certified ray
classification is `CsgRothRayClassifier`.
*/
template <class Payload>
class CsgSegmentClassification {
private:
    java::ArrayList<CsgSegmentEvent<Payload> > events;
    bool initialInside;
    bool currentInside;
public:
    // An empty classification starting inside or outside, with room for
    // expectedEvents events.
    CsgSegmentClassification(bool initialInside, long int expectedEvents);
    // Appends the next sorted event of a leaf, toggling the state.
    void append(double t, const Payload& payload);
    const java::ArrayList<CsgSegmentEvent<Payload> >& getEvents() const;
    bool isInitialInside() const;
    static CsgSegmentClassification<Payload> merge(
        const CsgSegmentClassification<Payload>& left,
        const CsgSegmentClassification<Payload>& right,
        CsgOperation operation);
};

#endif
