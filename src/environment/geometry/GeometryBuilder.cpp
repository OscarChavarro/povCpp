#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"

#include "environment/geometry/GeometryBuilder.h"

PolynomialShape *
GeometryBuilder::getPolyShape(int order, const int *termCounts)
{
    (void)termCounts;

    PolynomialShape *newShape = new PolynomialShape(order);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

CSG *
GeometryBuilder::getCsgUnion()
{
    CSG *newShape = new CSG(BooleanSetOperations::UNION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

CSG *
GeometryBuilder::getCsgIntersection()
{
    CSG *newShape = new CSG(BooleanSetOperations::INTERSECTION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}
