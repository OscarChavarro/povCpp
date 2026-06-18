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

Light *
ModelBuilder::promoteToSpotLight(Light *old)
{
    SpotLight *newLight = new SpotLight();
    if (newLight == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
        return old;
    }
    old->copyStateInto(newLight);
    delete old;
    return newLight;
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
    int i;

    newShape = new PolynomialShape;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }

    newShape->setTransformation(nullptr);
    newShape->setTransformationInverse(nullptr);
    newShape->setInverted(0);
    newShape->setOrder(order);
    newShape->setSturmFlag(0);
    newShape->setCoeffs(new double[termCounts[order]]);
    if (newShape->getCoeffs() == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate coefficients for POLY\n");
    }
    for (i = 0; i < termCounts[order]; i++) {
        newShape->getCoeffs()[i] = 0.0;
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

    newShape->setTransformation(nullptr);
    newShape->setTransformationInverse(nullptr);
    newShape->setInverted(false);
    return (newShape);
}

ParametricBiCubicPatch *
ModelBuilder::getBicubicPatchShape()
{
    ParametricBiCubicPatch *newShape;

    newShape = new ParametricBiCubicPatch;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }

    newShape->setUSteps(0);
    newShape->setVSteps(0);
    newShape->setIntersectionCount(0);
    newShape->setInterpolatedGrid((Vector3Dd **)nullptr);
    newShape->setInterpolatedNormals((Vector3Dd **)nullptr);
    newShape->setSmoothNormals((Vector3Dd **)nullptr);
    newShape->setInterpolatedD((double **)nullptr);
    newShape->setNodeTree(nullptr);
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
    newShape->setBoundingBox(ModelBuilder::getBoxShape());
    newShape->setMap(nullptr);
    newShape->setTransformation(new Matrix4x4d(Matrix4x4d::identityMatrix()));
    newShape->setTransformationInverse(
        new Matrix4x4d(Matrix4x4d::identityMatrix()));
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

    newShape = ModelBuilder::getCsgShape();
    newShape->setGeometryType(GeometryTypes::CSG_UNION_TYPE);
    return (newShape);
}

CSG *
ModelBuilder::getCsgIntersection()
{
    CSG *newShape;

    newShape = ModelBuilder::getCsgShape();
    newShape->setGeometryType(GeometryTypes::CSG_INTERSECTION_TYPE);
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

    newColor = new ColorRgba;
    if (newColor == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate color\n");
    }

    newColor->setR(0.0); newColor->setG(0.0); newColor->setB(0.0); newColor->setA(0);
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

    *newVector = Vector3Dd(0.0, 0.0, 0.0);
    return (newVector);
}

double *
ModelBuilder::getFloat()
{
    double *newFloat;

    newFloat = new double;
    if (newFloat == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate float\n");
    }

    *newFloat = 0.0;
    return (newFloat);
}
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
