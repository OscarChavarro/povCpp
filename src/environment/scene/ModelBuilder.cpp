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

Composite *
ModelBuilder::getCompositeObject()
{
    Composite *newComposite;

    newComposite = new Composite;
    if (newComposite == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object\n");
    }

    newComposite->simpleBodies = nullptr;
    newComposite->type = GeometryTypes::COMPOSITE_TYPE;
    newComposite->nextObject = nullptr;
    newComposite->boundingShapes = nullptr;
    newComposite->clippingShapes = nullptr;
    newComposite->methods = &Composite::compositeMethodTable;
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

    *&(newShape->Center) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->Radius = 1.0;
    newShape->radiusSquared = 1.0;
    newShape->inverseRadius = 1.0;
    newShape->geometryType = GeometryTypes::SPHERE_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Sphere::methodTable;
    newShape->VPCached = false;
    newShape->Inverted = false;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
    return (newShape);
}

Light *
ModelBuilder::getLightSourceShape()
{
    Light *newShape;

    newShape = new Light;
    if (newShape == nullptr) {
        Logger::reportMessage("ModelBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate shape\n");
    }
    *&(newShape->Center) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->pointsAt) = Vector3Dd(0.0, 0.0, 1.0);
    newShape->geometryType = GeometryTypes::POINT_LIGHT_TYPE;
    newShape->methods = &Light::methodTable;
    newShape->nextObject = nullptr;
    newShape->Inverted = false;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
    newShape->Coeff = 10.0;
    newShape->Radius = 0.35;
    newShape->Falloff = 0.35;
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

    *&(newShape->object2Terms) = Vector3Dd(1.0, 1.0, 1.0);
    *&(newShape->objectMixedTerms) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->objectTerms) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->objectConstant = 1.0;
    newShape->objectVpConstant = HUGE_VAL;
    newShape->constantCached = false;
    newShape->nonZeroSquareTerm = false;
    newShape->geometryType = GeometryTypes::QUARTIC_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Quadric::methodTable;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
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

    newShape->geometryType = GeometryTypes::POLY_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &PolynomialShape::methodTable;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
    newShape->transformation = nullptr;
    newShape->transformationInverse = nullptr;
    newShape->Inverted = 0;
    newShape->Order = order;
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
    newShape->geometryType = GeometryTypes::BOX_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Box::methodTable;
    newShape->Inverted = false;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
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
    newShape->geometryType = GeometryTypes::BLOB_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Blob::methodTable;
    newShape->Inverted = false;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
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

    newShape->geometryType = GeometryTypes::BICUBIC_PATCH_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &ParametricBiCubicPatch::methodTable;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
    newShape->uSteps = 0;
    newShape->vSteps = 0;
    newShape->intersectionCount = 0;
    newShape->Interpolated_Grid = (Vector3Dd **)nullptr;
    newShape->Interpolated_Normals = (Vector3Dd **)nullptr;
    newShape->Smooth_Normals = (Vector3Dd **)nullptr;
    newShape->Interpolated_D = (double **)nullptr;
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
    newShape->bounding_box = ModelBuilder::getBoxShape();
    newShape->Map = nullptr;
    newShape->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
    newShape->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    newShape->geometryType = GeometryTypes::HEIGHT_FIELD_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &HeightField::methodTable;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
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
    newShape->Distance = 0.0;
    newShape->geometryType = GeometryTypes::PLANE_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &InfinitePlane::methodTable;
    newShape->VPCached = 0;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
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
    *&(newShape->P1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->P2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->P3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Inverted = false;
    newShape->geometryType = GeometryTypes::TRIANGLE_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Triangle::methodTable;
    newShape->VPCached = false;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
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
    *&(newShape->P1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->P2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->P3) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N1) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N2) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->geometryType = GeometryTypes::SMOOTH_TRIANGLE_TYPE;
    newShape->Inverted = false;
    newShape->nextObject = nullptr;
    newShape->methods = &Triangle::smoothMethodTable;
    newShape->VPCached = 0;
    newShape->material = nullptr;
    newShape->shapeColor = nullptr;
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

    newShape->nextObject = nullptr;
    newShape->Shapes = nullptr;
    return (newShape);
}

CSG *
ModelBuilder::getCsgUnion()
{
    CSG *newShape;

    newShape = ModelBuilder::getCsgShape();
    newShape->methods = &CSG::unionMethodTable;
    newShape->geometryType = GeometryTypes::CSG_UNION_TYPE;
    return (newShape);
}

CSG *
ModelBuilder::getCsgIntersection()
{
    CSG *newShape;

    newShape = ModelBuilder::getCsgShape();
    newShape->methods = &CSG::intersectionMethodTable;
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
#include "java/util/PriorityQueue.txx"
