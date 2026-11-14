#include "environment/geometry/volume/BoxInvertedCrossingSink.h"

BoxInvertedCrossingSink::BoxInvertedCrossingSink(RayCrossingSink& target) : destination(target) {}
void BoxInvertedCrossingSink::crossing(const RayCrossing& event)
{
    RayCrossing reversed(event);
    reversed.normalReversed = !reversed.normalReversed;
    if (reversed.kind == RayCrossing::Kind::Entering) reversed.kind = RayCrossing::Kind::Leaving;
    else if (reversed.kind == RayCrossing::Kind::Leaving) reversed.kind = RayCrossing::Kind::Entering;
    destination.crossing(reversed);
}
