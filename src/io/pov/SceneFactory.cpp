#include "io/pov/Parse.h"
#include "common/Frame.h"
#include "common/Matrices.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "common/VectorOps.h"
#include "io/Gif.h"
#include "io/Iff.h"
#include "io/Targa.h"
#include "io/Dump.h"
#include "render/Render.h"

#include "geom/Bezier.h"
#include "geom/Blob.h"
#include "geom/Boxes.h"
#include "geom/Csg.h"
#include "geom/HeightField.h"
#include "geom/Light.h"
#include "geom/Objects.h"
#include "geom/Planes.h"
#include "geom/PolynomialShape.h"
#include "geom/Quadrics.h"
#include "geom/Spheres.h"
#include "geom/Triangle.h"
#include "geom/Viewpoint.h"

extern ReservedWord globalReservedWords[];
extern double antialiasThreshold;
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

extern RenderFrame *parsingFramePtr;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;


/* Allocate and initialize a composite object. */
Composite *
SceneFactory::getCompositeObject()
{
    Composite *newComposite;

    newComposite = new Composite;
    if (newComposite == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate object");
    }

    newComposite->Objects = nullptr;
    newComposite->Next_Object = nullptr;
    /*  New_Composite -> Next_Light_Source = NULL;*/
    newComposite->Bounding_Shapes = nullptr;
    newComposite->Clipping_Shapes = nullptr;
    newComposite->Type = COMPOSITE_TYPE;
    newComposite->methods = &Composite_Methods;
    return (newComposite);
}

/* Allocate and initialize a sphere. */
Sphere *
SceneFactory::getSphereShape()
{
    Sphere *newShape;

    newShape = new Sphere();
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    VectorOps::makeVector(&(newShape->Center), 0.0, 0.0, 0.0);
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

/* Allocate and initialize a light source. */
/* A point light source has no shape, but we'll treat it like it does */
Light *
SceneFactory::getLightSourceShape()
{
    Light *newShape;

    newShape = new Light;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }
    VectorOps::makeVector(&(newShape->Center), 0.0, 0.0, 0.0);
    VectorOps::makeVector(&(newShape->Points_At), 0.0, 0.0, 1.0);
    newShape->Type = POINT_LIGHT_TYPE;
    newShape->methods = &Point_Methods;
    newShape->Next_Object = nullptr;
    newShape->Inverted = FALSE; /* needed so CSG routines don't blow up */
    newShape->Shape_Texture = nullptr;     /* always NULL */
    newShape->Shape_Colour = SceneFactory::getColour(); /* becomes light colour */
    Color::makeColor(newShape->Shape_Colour, 1.0, 1.0, 1.0);
    newShape->Shape_Colour->Alpha = 0.0;
    newShape->Coeff = 10.0;
    newShape->Radius = 0.35;
    newShape->Falloff = 0.35;
    return (newShape);
}

/* Allocate and initialize a quadric surface. */
Quadric *
SceneFactory::getQuadricShape()
{
    Quadric *newShape;

    newShape = new Quadric;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    VectorOps::makeVector(&(newShape->Object_2_Terms), 1.0, 1.0, 1.0);
    VectorOps::makeVector(&(newShape->Object_Mixed_Terms), 0.0, 0.0, 0.0);
    VectorOps::makeVector(&(newShape->Object_Terms), 0.0, 0.0, 0.0);
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

/* Allocate and initialize a polynomial surface. */
PolynomialShape *
SceneFactory::getPolyShape(int order)
{
    PolynomialShape *newShape;
    int i;

    newShape = new PolynomialShape;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
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
        ParseErrorReporter::Error("Out of memory. Cannot allocate coefficients for POLY");
    }
    for (i = 0; i < termCounts[order]; i++) {
        newShape->Coeffs[i] = 0.0;
    }
    return (newShape);
}

/* Allocate and initialize a box. */
Box *
SceneFactory::getBoxShape()
{
    Box *newShape;

    newShape = new Box;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    VectorOps::makeVector(&(newShape->bounds[0]), -1.0, -1.0, -1.0);
    VectorOps::makeVector(&(newShape->bounds[1]), 1.0, 1.0, 1.0);
    newShape->Transform = nullptr;
    newShape->Type = BOX_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Box_Methods;
    newShape->Inverted = FALSE;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

/* Allocate a blob. */
Blob *
SceneFactory::getBlobShape()
{
    Blob *newShape;

    newShape = new Blob;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
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

/* Allocate and initialize a bicubic patch surface. */
BicubicPatch *
SceneFactory::getBicubicPatchShape()
{
    BicubicPatch *newShape;

    newShape = new BicubicPatch;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    newShape->Type = BICUBIC_PATCH_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Bicubic_Patch_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->U_Steps = 0;
    newShape->V_Steps = 0;
    newShape->Intersection_Count = 0;
    newShape->Interpolated_Grid = (Vector3D **)nullptr;
    newShape->Interpolated_Normals = (Vector3D **)nullptr;
    newShape->Smooth_Normals = (Vector3D **)nullptr;
    newShape->Interpolated_D = (double **)nullptr;
    return (newShape);
}

/* Allocate and intialize a Height Field */
HeightField *
SceneFactory::getHeightFieldShape()
{
    HeightField *newShape;

    newShape = new HeightField;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }
    newShape->bounding_box = SceneFactory::getBoxShape();
    newShape->Map = nullptr;
    newShape->transformation = Transformation::getTransformation();
    newShape->Type = HEIGHT_FIELD_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Height_Field_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

/* Allocate and initialize a plane. */
InfinitePlane *
SceneFactory::getPlaneShape()
{
    InfinitePlane *newShape;

    newShape = new InfinitePlane;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    VectorOps::makeVector(&(newShape->Normal_Vector), 0.0, 1.0, 0.0);
    newShape->Distance = 0.0;
    newShape->Type = PLANE_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Plane_Methods;
    newShape->VPCached = 0;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

/* Allocate and initialize a triangle. */
Triangle *
SceneFactory::getTriangleShape()
{
    Triangle *newShape;

    newShape = new Triangle;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    VectorOps::makeVector(&(newShape->Normal_Vector), 0.0, 1.0, 0.0);
    VectorOps::makeVector(&(newShape->P1), 0.0, 0.0, 0.0);
    VectorOps::makeVector(&(newShape->P2), 1.0, 0.0, 0.0);
    VectorOps::makeVector(&(newShape->P3), 0.0, 1.0, 0.0);
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

/* Allocate and initialize a smooth triangle. */
SmoothTriangle *
SceneFactory::getSmoothTriangleShape()
{
    SmoothTriangle *newShape;

    newShape = new SmoothTriangle;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    VectorOps::makeVector(&(newShape->Normal_Vector), 0.0, 1.0, 0.0);
    VectorOps::makeVector(&(newShape->P1), 0.0, 0.0, 0.0);
    VectorOps::makeVector(&(newShape->P2), 1.0, 0.0, 0.0);
    VectorOps::makeVector(&(newShape->P3), 0.0, 1.0, 0.0);
    VectorOps::makeVector(&(newShape->N1), 0.0, 1.0, 0.0);
    VectorOps::makeVector(&(newShape->N2), 0.0, 1.0, 0.0);
    VectorOps::makeVector(&(newShape->N3), 0.0, 1.0, 0.0);
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
SceneFactory::getCsgShape()
{
    CSG *newShape;

    newShape = new CSG;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    newShape->Parent_Object = nullptr;
    newShape->Next_Object = nullptr;
    newShape->Shapes = nullptr;
    return (newShape);
}

CSG *
SceneFactory::getCsgUnion()
{
    CSG *newShape;

    newShape = SceneFactory::getCsgShape();
    newShape->methods = &CSG_Union_Methods;
    newShape->Type = CSG_UNION_TYPE;
    return (newShape);
}

CSG *
SceneFactory::getCsgIntersection()
{
    CSG *newShape;

    newShape = SceneFactory::getCsgShape();
    newShape->methods = &CSG_Intersection_Methods;
    newShape->Type = CSG_INTERSECTION_TYPE;
    return (newShape);
}

Viewpoint *
SceneFactory::getViewpoint()
{
    Viewpoint *newViewpoint;

    newViewpoint = new Viewpoint;
    if (newViewpoint == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate viewpoint");
    }

    newViewpoint->initializeDefaults();
    return (newViewpoint);
}

RGBAColor *
SceneFactory::getColour()
{
    RGBAColor *newColour;

    newColour = new RGBAColor;
    if (newColour == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate colour");
    }

    Color::makeColor(newColour, 0.0, 0.0, 0.0);
    return (newColour);
}

Vector3D *
SceneFactory::getVector()
{
    Vector3D *newVector;

    newVector = new Vector3D;
    if (newVector == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate vector");
    }

    newVector->x = 0.0;
    newVector->y = 0.0;
    newVector->z = 0.0;
    return (newVector);
}

double *
SceneFactory::getFloat()
{
    double *newFloat;

    newFloat = new double;
    if (newFloat == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate float");
    }

    *newFloat = 0.0;
    return (newFloat);
}

BicubicPatch *
BicubicPatch::getBicubicPatchShape()
{
    BicubicPatch *newShape;

    newShape = new BicubicPatch;
    if (newShape == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate shape");
    }

    newShape->Type = BICUBIC_PATCH_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Bicubic_Patch_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    newShape->U_Steps = 0;
    newShape->V_Steps = 0;
    newShape->Intersection_Count = 0;
    newShape->Interpolated_Grid = (Vector3D **)nullptr;
    newShape->Interpolated_Normals = (Vector3D **)nullptr;
    newShape->Smooth_Normals = (Vector3D **)nullptr;
    newShape->Interpolated_D = (double **)nullptr;
    return (newShape);
}

/* Parse a float.  Doesn't handle exponentiation. */
