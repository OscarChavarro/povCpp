#ifndef VSDK_CSG_SEGMENT_CLASSIFICATION_TXX
#define VSDK_CSG_SEGMENT_CLASSIFICATION_TXX

#include "vsdk/toolkit/environment/scene/csg/CsgSegmentClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "java/util/ArrayList.txx"

template <class Payload>
CsgSegmentClassification<Payload>::CsgSegmentClassification(bool initialInside, long int expectedEvents) :
    events(expectedEvents), initialInside(initialInside), currentInside(initialInside)
{
}

template <class Payload>
void CsgSegmentClassification<Payload>::append(double t, const Payload& payload)
{
    currentInside = !currentInside;
    events.add(CsgSegmentEvent<Payload>(t, currentInside, payload));
}

template <class Payload>
const java::ArrayList<CsgSegmentEvent<Payload> >& CsgSegmentClassification<Payload>::getEvents() const
{
    return events;
}

template <class Payload>
bool CsgSegmentClassification<Payload>::isInitialInside() const
{
    return initialInside;
}

template <class Payload>
CsgSegmentClassification<Payload> CsgSegmentClassification<Payload>::merge(
    const CsgSegmentClassification<Payload>& left,
    const CsgSegmentClassification<Payload>& right,
    CsgOperation operation)
{
    const java::ArrayList<CsgSegmentEvent<Payload> >& leftEvents = left.events;
    const java::ArrayList<CsgSegmentEvent<Payload> >& rightEvents = right.events;
    bool insideLeft = left.initialInside;
    bool insideRight = right.initialInside;
    const bool initialCombined = CsgMembershipRule::combine(operation, insideLeft, insideRight);
    CsgSegmentClassification<Payload> result(initialCombined, leftEvents.size() + rightEvents.size());

    bool previousCombined = initialCombined;
    long int i = 0;
    long int j = 0;
    while (i < leftEvents.size() || j < rightEvents.size()) {
        bool takeLeft;
        if (j >= rightEvents.size()) {
            takeLeft = true;
        } else if (i >= leftEvents.size()) {
            takeLeft = false;
        } else {
            takeLeft = leftEvents[i].getT() <= rightEvents[j].getT();
        }
        const CsgSegmentEvent<Payload>& event = takeLeft ? leftEvents[i] : rightEvents[j];
        if (takeLeft) {
            insideLeft = event.isEntering();
            i++;
        } else {
            insideRight = event.isEntering();
            j++;
        }
        const bool newCombined = CsgMembershipRule::combine(operation, insideLeft, insideRight);
        if (newCombined != previousCombined) {
            result.events.add(CsgSegmentEvent<Payload>(event.getT(), newCombined, event.getPayload()));
            previousCombined = newCombined;
        }
    }
    result.currentInside = previousCombined;
    return result;
}

#endif
