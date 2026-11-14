#include "TestCrossingCollector.h"
#include "java/util/ArrayList.txx"

TestCrossingCollector::~TestCrossingCollector()
{
    for (long int i = 0; i < hits.size(); ++i) delete hits[i];
}

void TestCrossingCollector::crossing(const RayCrossing& hit)
{
    hits.add(new RayCrossing(hit));
}
