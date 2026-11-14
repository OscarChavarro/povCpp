#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

#include "io/pov/geometry/GeometryBuilder.h"

PolynomialSurface *
GeometryBuilder::getPolyShape(int order)
{
    PolynomialSurface *newShape = new PolynomialSurface(order, false);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

ConstructiveSolidGeometry *
GeometryBuilder::getCsgUnion()
{
    ConstructiveSolidGeometry *newShape = new ConstructiveSolidGeometry(CsgOperation::UNION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

ConstructiveSolidGeometry *
GeometryBuilder::getCsgIntersection()
{
    ConstructiveSolidGeometry *newShape = new ConstructiveSolidGeometry(CsgOperation::INTERSECTION);
    if (newShape == nullptr) {
        Logger::reportMessage("GeometryBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}
