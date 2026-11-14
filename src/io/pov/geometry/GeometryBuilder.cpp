#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.h"

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
    ConstructiveSolidGeometry *newShape = new ConstructiveSolidGeometryByMorganRules(CsgOperation::UNION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

ConstructiveSolidGeometry *
GeometryBuilder::getCsgIntersection()
{
    ConstructiveSolidGeometry *newShape = new ConstructiveSolidGeometryByMorganRules(CsgOperation::INTERSECTION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}
