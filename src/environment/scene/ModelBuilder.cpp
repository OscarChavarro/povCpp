#include "environment/scene/ModelBuilder.h"

#include "common/LegacyBoolean.h"
#include "processing/PolynomialConstants.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/color/Color.h"
#include "common/logger/Logger.h"
#include "environment/camera/Camera.h"
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
#include "environment/light/Light.h"

Composite *
ModelBuilder::getCompositeObject()
{
    Composite *newComposite;

    newComposite = new Composite;
    if (newComposite == nullptr) {
        Logger::error("Out of memory. Cannot allocate object\n");
        exit(1);
    }

    newComposite->Objects = nullptr;
    newComposite->nextObject = nullptr;
    newComposite->boundingShapes = nullptr;
    newComposite->clippingShapes = nullptr;
    newComposite->Type = GeometryOperations::COMPOSITE_TYPE;
    newComposite->methods = &Composite::compositeMethodTable;
    return (newComposite);
}

Sphere *
ModelBuilder::getSphereShape()
{
    Sphere *newShape;

    newShape = new Sphere();
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->Center) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->Radius = 1.0;
    newShape->radiusSquared = 1.0;
    newShape->inverseRadius = 1.0;
    newShape->Type = GeometryOperations::SPHERE_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Sphere::methodTable;
    newShape->VPCached = LegacyBoolean::FALSE_VALUE;
    newShape->Inverted = LegacyBoolean::FALSE_VALUE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

Light *
ModelBuilder::getLightSourceShape()
{
    Light *newShape;

    newShape = new Light;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }
    *&(newShape->Center) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->pointsAt) = Vector3Dd(0.0, 0.0, 1.0);
    newShape->Type = GeometryOperations::POINT_LIGHT_TYPE;
    newShape->methods = &Light::methodTable;
    newShape->nextObject = nullptr;
    newShape->Inverted = LegacyBoolean::FALSE_VALUE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = ModelBuilder::getColour();
    Color::makeColor(newShape->Shape_Colour, 1.0, 1.0, 1.0);
    newShape->Shape_Colour->Alpha = 0.0;
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
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->object2Terms) = Vector3Dd(1.0, 1.0, 1.0);
    *&(newShape->objectMixedTerms) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->objectTerms) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->objectConstant = 1.0;
    newShape->objectVpConstant = HUGE_VAL;
    newShape->constantCached = LegacyBoolean::FALSE_VALUE;
    newShape->nonZeroSquareTerm = LegacyBoolean::FALSE_VALUE;
    newShape->Type = GeometryOperations::QUADRIC_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Quadric::methodTable;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

PolynomialShape *
ModelBuilder::getPolyShape(int order, const int *termCounts)
{
    PolynomialShape *newShape;
    int i;

    newShape = new PolynomialShape;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    newShape->Type = GeometryOperations::POLY_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &PolynomialShape::methodTable;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->Transform = nullptr;
    newShape->Inverted = 0;
    newShape->Order = order;
    newShape->sturmFlag = 0;
    newShape->Coeffs = new double[termCounts[order]];
    if (newShape->Coeffs == nullptr) {
        Logger::error(
            "Out of memory. Cannot allocate coefficients for POLY\n");
        exit(1);
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
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->bounds[0]) = Vector3Dd(-1.0, -1.0, -1.0);
    *&(newShape->bounds[1]) = Vector3Dd(1.0, 1.0, 1.0);
    newShape->Transform = nullptr;
    newShape->Type = GeometryOperations::BOX_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Box::methodTable;
    newShape->Inverted = LegacyBoolean::FALSE_VALUE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

Blob *
ModelBuilder::getBlobShape()
{
    Blob *newShape;

    newShape = new Blob;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    newShape->Transform = nullptr;
    newShape->Type = GeometryOperations::BLOB_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Blob::methodTable;
    newShape->Inverted = LegacyBoolean::FALSE_VALUE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

ParametricBiCubicPatch *
ModelBuilder::getBicubicPatchShape()
{
    ParametricBiCubicPatch *newShape;

    newShape = new ParametricBiCubicPatch;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    newShape->Type = GeometryOperations::BICUBIC_PATCH_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &ParametricBiCubicPatch::methodTable;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
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
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }
    newShape->bounding_box = ModelBuilder::getBoxShape();
    newShape->Map = nullptr;
    newShape->transformation = Transformation::getTransformation();
    newShape->Type = GeometryOperations::HEIGHT_FIELD_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &HeightField::methodTable;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

InfinitePlane *
ModelBuilder::getPlaneShape()
{
    InfinitePlane *newShape;

    newShape = new InfinitePlane;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->normalVector) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Type = GeometryOperations::PLANE_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &InfinitePlane::methodTable;
    newShape->VPCached = 0;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

Triangle *
ModelBuilder::getTriangleShape()
{
    Triangle *newShape;

    newShape = new Triangle;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->normalVector) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->P1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->P2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->P3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Inverted = LegacyBoolean::FALSE_VALUE;
    newShape->Type = GeometryOperations::TRIANGLE_TYPE;
    newShape->nextObject = nullptr;
    newShape->methods = &Triangle::methodTable;
    newShape->VPCached = LegacyBoolean::FALSE_VALUE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->degenerateFlag = LegacyBoolean::FALSE_VALUE;
    return (newShape);
}

SmoothTriangle *
ModelBuilder::getSmoothTriangleShape()
{
    SmoothTriangle *newShape;

    newShape = new SmoothTriangle;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->normalVector) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->P1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->P2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->P3) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N1) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N2) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Type = GeometryOperations::SMOOTH_TRIANGLE_TYPE;
    newShape->Inverted = LegacyBoolean::FALSE_VALUE;
    newShape->nextObject = nullptr;
    newShape->methods = &Triangle::smoothMethodTable;
    newShape->VPCached = 0;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->degenerateFlag = LegacyBoolean::FALSE_VALUE;
    return (newShape);
}

CSG *
ModelBuilder::getCsgShape()
{
    CSG *newShape;

    newShape = new CSG;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
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
    newShape->Type = GeometryOperations::CSG_UNION_TYPE;
    return (newShape);
}

CSG *
ModelBuilder::getCsgIntersection()
{
    CSG *newShape;

    newShape = ModelBuilder::getCsgShape();
    newShape->methods = &CSG::intersectionMethodTable;
    newShape->Type = GeometryOperations::CSG_INTERSECTION_TYPE;
    return (newShape);
}

Camera *
ModelBuilder::getCamera()
{
    Camera *newViewpoint;

    newViewpoint = new Camera;
    if (newViewpoint == nullptr) {
        Logger::error("Out of memory. Cannot allocate viewpoint\n");
        exit(1);
    }

    newViewpoint->initializeDefaults();
    return (newViewpoint);
}

RGBAColor *
ModelBuilder::getColour()
{
    RGBAColor *newColour;

    newColour = new RGBAColor;
    if (newColour == nullptr) {
        Logger::error("Out of memory. Cannot allocate colour\n");
        exit(1);
    }

    Color::makeColor(newColour, 0.0, 0.0, 0.0);
    return (newColour);
}

Vector3Dd *
ModelBuilder::getVector()
{
    Vector3Dd *newVector;

    newVector = new Vector3Dd;
    if (newVector == nullptr) {
        Logger::error("Out of memory. Cannot allocate vector\n");
        exit(1);
    }

    newVector->x = 0.0;
    newVector->y = 0.0;
    newVector->z = 0.0;
    return (newVector);
}

double *
ModelBuilder::getFloat()
{
    double *newFloat;

    newFloat = new double;
    if (newFloat == nullptr) {
        Logger::error("Out of memory. Cannot allocate float\n");
        exit(1);
    }

    *newFloat = 0.0;
    return (newFloat);
}
