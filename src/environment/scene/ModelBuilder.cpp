#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/geometry/elements/Triangle.h"
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
#include "environment/camera/Camera.h"
#include "environment/light/Light.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/scene/TranslatedBody.h"

TranslatedBody *
ModelBuilder::wrap(Geometry *geometry)
{
    TranslatedBody *body;

    body = new TranslatedBody;
    if (body == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate body\n");
    }
    body->geometry = geometry;
    body->material = nullptr;
    body->shapeColor = nullptr;
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

    *&(newShape->center) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->radius = 1.0;
    newShape->radiusSquared = 1.0;
    newShape->inverseRadius = 1.0;
    newShape->vpCached = false;
    newShape->inverted = false;
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
    newLight->geometryType = GeometryTypes::SPOT_LIGHT_TYPE;
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

    *&(newShape->object2Terms) = Vector3Dd(1.0, 1.0, 1.0);
    *&(newShape->objectMixedTerms) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->objectTerms) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->objectConstant = 1.0;
    newShape->objectVpConstant = HUGE_VAL;
    newShape->constantCached = false;
    newShape->nonZeroSquareTerm = false;
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

    newShape->transformation = nullptr;
    newShape->transformationInverse = nullptr;
    newShape->inverted = 0;
    newShape->order = order;
    newShape->sturmFlag = 0;
    newShape->Coeffs = new double[termCounts[order]];
    if (newShape->Coeffs == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate coefficients for POLY\n");
    }
    for (i = 0; i < termCounts[order]; i++) {
        newShape->Coeffs[i] = 0.0;
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

    *&(newShape->bounds[0]) = Vector3Dd(-1.0, -1.0, -1.0);
    *&(newShape->bounds[1]) = Vector3Dd(1.0, 1.0, 1.0);
    newShape->transformation = nullptr;
    newShape->transformationInverse = nullptr;
    newShape->inverted = false;
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

    newShape->transformation = nullptr;
    newShape->transformationInverse = nullptr;
    newShape->inverted = false;
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

    newShape->uSteps = 0;
    newShape->vSteps = 0;
    newShape->intersectionCount = 0;
    newShape->interpolatedGrid = (Vector3Dd **)nullptr;
    newShape->interpolatedNormals = (Vector3Dd **)nullptr;
    newShape->smoothNormals = (Vector3Dd **)nullptr;
    newShape->interpolatedD = (double **)nullptr;
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
    newShape->boundingBox = ModelBuilder::getBoxShape();
    newShape->Map = nullptr;
    newShape->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
    newShape->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
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

    *&(newShape->normalVector) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->distance = 0.0;
    newShape->vpCached = 0;
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

    *&(newShape->normalVector) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->p1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->p2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->p3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->distance = 0.0;
    newShape->inverted = false;
    newShape->vpCached = false;
    newShape->degenerateFlag = false;
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

    *&(newShape->normalVector) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->p1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->p2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->p3) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->n1) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->n2) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->n3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->distance = 0.0;
    newShape->inverted = false;
    newShape->vpCached = 0;
    newShape->degenerateFlag = false;
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
    newShape->geometryType = GeometryTypes::CSG_UNION_TYPE;
    return (newShape);
}

CSG *
ModelBuilder::getCsgIntersection()
{
    CSG *newShape;

    newShape = ModelBuilder::getCsgShape();
    newShape->geometryType = GeometryTypes::CSG_INTERSECTION_TYPE;
    return (newShape);
}

Camera *
ModelBuilder::getCamera()
{
    Camera *newViewpoint;

    newViewpoint = new Camera;
    if (newViewpoint == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate viewpoint\n");
    }

    newViewpoint->initializeDefaults();
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
