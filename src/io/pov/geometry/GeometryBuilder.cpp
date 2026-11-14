#include "vsdk/toolkit/common/logging/Logger.h"

#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.h"

#include "io/pov/geometry/GeometryBuilder.h"

PolynomialShape *
GeometryBuilder::getPolyShape(int order)
{
    PolynomialShape *newShape = new PolynomialShape(order);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

ConstructiveSolidGeometry *
GeometryBuilder::getCsgUnion()
{
    ConstructiveSolidGeometry *newShape = new ConstructiveSolidGeometryByMorganRules(BooleanSetOperations::UNION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

ConstructiveSolidGeometry *
GeometryBuilder::getCsgIntersection()
{
    ConstructiveSolidGeometry *newShape = new ConstructiveSolidGeometryByMorganRules(BooleanSetOperations::INTERSECTION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}
