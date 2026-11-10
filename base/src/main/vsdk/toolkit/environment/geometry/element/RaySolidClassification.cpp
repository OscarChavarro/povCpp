#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"

RaySolidClassification::RaySolidClassification() {}

void RaySolidClassification::clear()
{
    boundaries.clear();
    insideSegments.clear();
}

RaySolidClassification::~RaySolidClassification() {}

RaySolidClassification::RaySolidClassification(const RaySolidClassification& other)
    : boundaries(other.boundaries), insideSegments(other.insideSegments) {}

RaySolidClassification& RaySolidClassification::operator=(const RaySolidClassification& other)
{
    if (this != &other) {
        boundaries = other.boundaries;
        insideSegments = other.insideSegments;
    }
    return *this;
}
