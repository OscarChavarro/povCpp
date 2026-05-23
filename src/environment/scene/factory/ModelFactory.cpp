#include "environment/scene/factory/ModelFactory.h"

#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "common/color/Color.h"
#include "common/logger/Logger.h"
#include "environment/camera/Viewpoint.h"
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

extern int termCounts[MAX_ORDER + 1];

Composite *
ModelFactory::getCompositeObject()
{
    Composite *newComposite;

    newComposite = new Composite;
    if (newComposite == nullptr) {
        Logger::error("Out of memory. Cannot allocate object\n");
        exit(1);
    }

    newComposite->Objects = nullptr;
    newComposite->Next_Object = nullptr;
    newComposite->Bounding_Shapes = nullptr;
    newComposite->Clipping_Shapes = nullptr;
    newComposite->Type = COMPOSITE_TYPE;
    newComposite->methods = &Composite_Methods;
    return (newComposite);
}

Sphere *
ModelFactory::getSphereShape()
{
    Sphere *newShape;

    newShape = new Sphere();
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->Center) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->Radius = 1.0;
    newShape->Radius_Squared = 1.0;
    newShape->Inverse_Radius = 1.0;
    newShape->Type = SPHERE_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Sphere_Methods;
    newShape->VPCached = FALSE;
    newShape->Inverted = FALSE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

Light *
ModelFactory::getLightSourceShape()
{
    Light *newShape;

    newShape = new Light;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }
    *&(newShape->Center) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->Points_At) = Vector3Dd(0.0, 0.0, 1.0);
    newShape->Type = POINT_LIGHT_TYPE;
    newShape->methods = &Point_Methods;
    newShape->Next_Object = nullptr;
    newShape->Inverted = FALSE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = ModelFactory::getColour();
    Color::makeColor(newShape->Shape_Colour, 1.0, 1.0, 1.0);
    newShape->Shape_Colour->Alpha = 0.0;
    newShape->Coeff = 10.0;
    newShape->Radius = 0.35;
    newShape->Falloff = 0.35;
    return (newShape);
}

Quadric *
ModelFactory::getQuadricShape()
{
    Quadric *newShape;

    newShape = new Quadric;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->Object_2_Terms) = Vector3Dd(1.0, 1.0, 1.0);
    *&(newShape->Object_Mixed_Terms) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->Object_Terms) = Vector3Dd(0.0, 0.0, 0.0);
    newShape->Object_Constant = 1.0;
    newShape->Object_VP_Constant = HUGE_VAL;
    newShape->Constant_Cached = FALSE;
    newShape->Non_Zero_Square_Term = FALSE;
    newShape->Type = QUADRIC_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Quadric_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

PolynomialShape *
ModelFactory::getPolyShape(int order)
{
    PolynomialShape *newShape;
    int i;

    newShape = new PolynomialShape;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    newShape->Type = POLY_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Poly_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->Transform = nullptr;
    newShape->Inverted = 0;
    newShape->Order = order;
    newShape->Sturm_Flag = 0;
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
ModelFactory::getBoxShape()
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
    newShape->Type = BOX_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Box_Methods;
    newShape->Inverted = FALSE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

Blob *
ModelFactory::getBlobShape()
{
    Blob *newShape;

    newShape = new Blob;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    newShape->Transform = nullptr;
    newShape->Type = BLOB_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Blob_Methods;
    newShape->Inverted = FALSE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

ParametricBiCubicPatch *
ModelFactory::getBicubicPatchShape()
{
    ParametricBiCubicPatch *newShape;

    newShape = new ParametricBiCubicPatch;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    newShape->Type = BICUBIC_PATCH_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Bicubic_Patch_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->U_Steps = 0;
    newShape->V_Steps = 0;
    newShape->Intersection_Count = 0;
    newShape->Interpolated_Grid = (Vector3Dd **)nullptr;
    newShape->Interpolated_Normals = (Vector3Dd **)nullptr;
    newShape->Smooth_Normals = (Vector3Dd **)nullptr;
    newShape->Interpolated_D = (double **)nullptr;
    return (newShape);
}

HeightField *
ModelFactory::getHeightFieldShape()
{
    HeightField *newShape;

    newShape = new HeightField;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }
    newShape->bounding_box = ModelFactory::getBoxShape();
    newShape->Map = nullptr;
    newShape->transformation = Transformation::getTransformation();
    newShape->Type = HEIGHT_FIELD_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Height_Field_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

InfinitePlane *
ModelFactory::getPlaneShape()
{
    InfinitePlane *newShape;

    newShape = new InfinitePlane;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->Normal_Vector) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Type = PLANE_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Plane_Methods;
    newShape->VPCached = 0;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

Triangle *
ModelFactory::getTriangleShape()
{
    Triangle *newShape;

    newShape = new Triangle;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->Normal_Vector) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->P1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->P2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->P3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Inverted = FALSE;
    newShape->Type = TRIANGLE_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Triangle_Methods;
    newShape->VPCached = FALSE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->Degenerate_Flag = FALSE;
    return (newShape);
}

SmoothTriangle *
ModelFactory::getSmoothTriangleShape()
{
    SmoothTriangle *newShape;

    newShape = new SmoothTriangle;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    *&(newShape->Normal_Vector) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->P1) = Vector3Dd(0.0, 0.0, 0.0);
    *&(newShape->P2) = Vector3Dd(1.0, 0.0, 0.0);
    *&(newShape->P3) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N1) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N2) = Vector3Dd(0.0, 1.0, 0.0);
    *&(newShape->N3) = Vector3Dd(0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Type = SMOOTH_TRIANGLE_TYPE;
    newShape->Inverted = FALSE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Smooth_Triangle_Methods;
    newShape->VPCached = 0;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->Degenerate_Flag = FALSE;
    return (newShape);
}

CSG *
ModelFactory::getCsgShape()
{
    CSG *newShape;

    newShape = new CSG;
    if (newShape == nullptr) {
        Logger::error("Out of memory. Cannot allocate shape\n");
        exit(1);
    }

    newShape->Next_Object = nullptr;
    newShape->Shapes = nullptr;
    return (newShape);
}

CSG *
ModelFactory::getCsgUnion()
{
    CSG *newShape;

    newShape = ModelFactory::getCsgShape();
    newShape->methods = &CSG_Union_Methods;
    newShape->Type = CSG_UNION_TYPE;
    return (newShape);
}

CSG *
ModelFactory::getCsgIntersection()
{
    CSG *newShape;

    newShape = ModelFactory::getCsgShape();
    newShape->methods = &CSG_Intersection_Methods;
    newShape->Type = CSG_INTERSECTION_TYPE;
    return (newShape);
}

Viewpoint *
ModelFactory::getViewpoint()
{
    Viewpoint *newViewpoint;

    newViewpoint = new Viewpoint;
    if (newViewpoint == nullptr) {
        Logger::error("Out of memory. Cannot allocate viewpoint\n");
        exit(1);
    }

    newViewpoint->initializeDefaults();
    return (newViewpoint);
}

RGBAColor *
ModelFactory::getColour()
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
ModelFactory::getVector()
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
ModelFactory::getFloat()
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
