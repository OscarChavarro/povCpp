#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/geometry/element/Triangle.h"

#include "environment/geometry/surface/InfinitePlane.h"

#include "environment/geometry/surface/parametric/ParametricPatch.h"

#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"

#include "environment/geometry/volume/polynomial/PolynomialShape.h"

#include "environment/light/Light.h"

#include "environment/camera/Camera.h"

#include "environment/scene/ModelBuilder.h"
#include "environment/geometry/SimpleBody.h"

SimpleBody *
ModelBuilder::wrap(Geometry *geometry)
{
    SimpleBody *body;
    body = new SimpleBody(geometry, nullptr, nullptr);
    if (body == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate body\n");
    }
    return (body);
}

Composite *
ModelBuilder::getCompositeObject()
{
    Composite *newComposite;

    newComposite = new Composite;
    if (newComposite == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object\n");
    }

    return (newComposite);
}

Sphere *
ModelBuilder::getSphereShape()
{
    Sphere *newShape;

    newShape = new Sphere();
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

Light *
ModelBuilder::getLightSourceShape()
{
    PointLight *newShape = new PointLight();
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

Quadric *
ModelBuilder::getQuadricShape()
{
    Quadric *newShape;

    newShape = new Quadric;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

PolynomialShape *
ModelBuilder::getPolyShape(int order, const int *termCounts)
{
    PolynomialShape *newShape;
    (void)termCounts;

    newShape = new PolynomialShape(order);
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

Box *
ModelBuilder::getBoxShape()
{
    Box *newShape;

    newShape = new Box;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

Blob *
ModelBuilder::getBlobShape()
{
    Blob *newShape;

    newShape = new Blob;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

HeightField *
ModelBuilder::getHeightFieldShape()
{
    HeightField *newShape;

    newShape = new HeightField;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

InfinitePlane *
ModelBuilder::getPlaneShape()
{
    InfinitePlane *newShape;

    newShape = new InfinitePlane;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

Triangle *
ModelBuilder::getTriangleShape()
{
    Triangle *newShape;

    newShape = new Triangle;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

SmoothTriangle *
ModelBuilder::getSmoothTriangleShape()
{
    SmoothTriangle *newShape;

    newShape = new SmoothTriangle;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

CSG *
ModelBuilder::getCsgShape()
{
    CSG *newShape;

    newShape = new CSG;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }

    return (newShape);
}

CSG *
ModelBuilder::getCsgUnion()
{
    CSG *newShape;

    newShape = new CSG(GeometryTypes::CSG_UNION_TYPE);
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

CSG *
ModelBuilder::getCsgIntersection()
{
    CSG *newShape;

    newShape = new CSG(GeometryTypes::CSG_INTERSECTION_TYPE);
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    return (newShape);
}

Camera *
ModelBuilder::getCamera()
{
    Camera *newViewpoint;

    newViewpoint = new Camera();
    if (newViewpoint == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate viewpoint\n");
    }
    return (newViewpoint);
}

ColorRgba *
ModelBuilder::getColor()
{
    ColorRgba *newColor;

    newColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
    if (newColor == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate color\n");
    }
    return (newColor);
}

Vector3Dd *
ModelBuilder::getVector()
{
    Vector3Dd *newVector;

    newVector = new Vector3Dd;
    if (newVector == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate vector\n");
    }
    return (newVector);
}

double *
ModelBuilder::getFloat()
{
    double *newFloat;

    newFloat = new double(0.0);
    if (newFloat == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate float\n");
    }
    return (newFloat);
}
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
