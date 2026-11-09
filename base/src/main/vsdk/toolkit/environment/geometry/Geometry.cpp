#include "vsdk/toolkit/environment/geometry/Geometry.h"

// Definitions needed by C++11 when the constants are bound to references
// (i.e. by the comparisons of the tests)
constexpr int Geometry::INSIDE;
constexpr int Geometry::LIMIT;
constexpr int Geometry::OUTSIDE;

void Geometry::doExtraInformation(const Ray&, double, RayHit*) {}
int Geometry::computeQuantitativeInvisibility(const Vector3Dd&, const Vector3Dd&) { return 0; }
int Geometry::doContainmentTest(const Vector3Dd&, double) { return OUTSIDE; }
