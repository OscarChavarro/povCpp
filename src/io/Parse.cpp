/****************************************************************************
 *                     parse.c
 *
 *  This module implements a parser for the scene description files.
 *
 *****************************************************************************/

#include "io/Parse.h"
#include "common/Frame.h"
#include "common/Matrices.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "io/Gif.h"
#include "io/Iff.h"
#include "render/Render.h"

/* This file implements a simple recursive-descent parser for reading the
input file.  */

static void linkShapes(Light *newObject, Light **field, Light **oldObjectList);
static void postProcessObject(SimpleBody *object);
static void postProcessShape(Geometry *shape);

extern DBL maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

Frame *parsingFramePtr;

#include "geom/Bezier.h"
#include "geom/Blob.h"
#include "geom/Boxes.h"
#include "geom/Csg.h"
#include "geom/HField.h"
#include "geom/Light.h"
#include "geom/Objects.h"
#include "geom/Planes.h"
#include "geom/Poly.h"
#include "geom/Quadrics.h"
#include "geom/Spheres.h"
#include "geom/Triangle.h"
#include "geom/ViewPnt.h"

extern ReservedWord globalReservedWords[];
extern DBL antialiasThreshold;

extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;

RGBAColorPaletteSpan *constructionMap =
    nullptr; /* moved here to allow reinitialization */

Constant constants[MAX_CONSTANTS];
int numberOfConstants;
int degenerateTriangles;

static inline void getExpectedToken(int tokenId)
{
    getToken();
    if (globalToken.Token_Id != tokenId) {
        parseError(tokenId);
    }
}

/* Parse the file into the given frame. */
void
Parse(Frame *framePtr)
{
    SimpleBody *object;
    parsingFramePtr = framePtr;

    degenerateTriangles = FALSE;
    tokenInit();
    frameInit();
    parseFrame();
    for (object = parsingFramePtr->Objects; object != nullptr;
         object = object->Next_Object) {
        postProcessObject(object);
    }
    if (degenerateTriangles) {
        fprintf(
            stderr, "Degenerate triangles were found and are being ignored.\n");
        /* exit(1); Let's ignore degen tri instead of blowing up. CdW */
    }
}

void
tokenInit()
{
    numberOfConstants = 0;
    /*
       Constants = new Constant[MAX_CONSTANTS];
    */
}

/* Set up the fields in the frame to default values. */
void
frameInit()
{
    Default_Texture = getTexture();
    parsingFramePtr->View_Point.initializeDefaults();
    parsingFramePtr->Light_Sources = nullptr;
    parsingFramePtr->Objects = nullptr;
    parsingFramePtr->Atmosphere_IOR = 1.0;
    parsingFramePtr->Antialias_Threshold = antialiasThreshold;
    parsingFramePtr->Fog_Distance = 0.0;
    Color::makeColor(&(parsingFramePtr->Fog_Colour), 0.0, 0.0, 0.0);
}

/* Allocate and initialize a composite object. */
Composite *
getCompositeObject()
{
    Composite *newComposite;

    newComposite = new Composite;
    if (newComposite == nullptr) {
        Error("Out of memory. Cannot allocate object");
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
getSphereShape()
{
    Sphere *newShape;

    newShape = new Sphere();
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }

    makeVector(&(newShape->Center), 0.0, 0.0, 0.0);
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
getLightSourceShape()
{
    Light *newShape;

    newShape = new Light;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }
    makeVector(&(newShape->Center), 0.0, 0.0, 0.0);
    makeVector(&(newShape->Points_At), 0.0, 0.0, 1.0);
    newShape->Type = POINT_LIGHT_TYPE;
    newShape->methods = &Point_Methods;
    newShape->Next_Object = nullptr;
    newShape->Inverted = FALSE; /* needed so CSG routines don't blow up */
    newShape->Shape_Texture = nullptr;     /* always NULL */
    newShape->Shape_Colour = getColour(); /* becomes light colour */
    Color::makeColor(newShape->Shape_Colour, 1.0, 1.0, 1.0);
    newShape->Shape_Colour->Alpha = 0.0;
    newShape->Coeff = 10.0;
    newShape->Radius = 0.35;
    newShape->Falloff = 0.35;
    return (newShape);
}

/* Allocate and initialize a quadric surface. */
Quadric *
getQuadricShape()
{
    Quadric *newShape;

    newShape = new Quadric;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }

    makeVector(&(newShape->Object_2_Terms), 1.0, 1.0, 1.0);
    makeVector(&(newShape->Object_Mixed_Terms), 0.0, 0.0, 0.0);
    makeVector(&(newShape->Object_Terms), 0.0, 0.0, 0.0);
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
Poly *
getPolyShape(int order)
{
    Poly *newShape;
    int i;

    newShape = new Poly;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
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
    newShape->Coeffs = new DBL[termCounts[order]];
    if (newShape->Coeffs == nullptr) {
        Error("Out of memory. Cannot allocate coefficients for POLY");
    }
    for (i = 0; i < termCounts[order]; i++) {
        newShape->Coeffs[i] = 0.0;
    }
    return (newShape);
}

/* Allocate and initialize a box. */
Box *
getBoxShape()
{
    Box *newShape;

    newShape = new Box;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }

    makeVector(&(newShape->bounds[0]), -1.0, -1.0, -1.0);
    makeVector(&(newShape->bounds[1]), 1.0, 1.0, 1.0);
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
getBlobShape()
{
    Blob *newShape;

    newShape = new Blob;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
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
getBicubicPatchShape()
{
    BicubicPatch *newShape;

    newShape = new BicubicPatch;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
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
    newShape->Interpolated_D = (DBL **)nullptr;
    return (newShape);
}

/* Allocate and intialize a Height Field */
HeightField *
getHeightFieldShape()
{
    HeightField *newShape;

    newShape = new HeightField;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }
    newShape->bounding_box = getBoxShape();
    newShape->Map = nullptr;
    newShape->transformation = getTransformation();
    newShape->Type = HEIGHT_FIELD_TYPE;
    newShape->Next_Object = nullptr;
    newShape->methods = &Height_Field_Methods;
    newShape->Shape_Texture = nullptr;
    newShape->Shape_Colour = nullptr;
    return (newShape);
}

/* Allocate and initialize a plane. */
InfinitePlane *
getPlaneShape()
{
    InfinitePlane *newShape;

    newShape = new InfinitePlane;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }

    makeVector(&(newShape->Normal_Vector), 0.0, 1.0, 0.0);
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
getTriangleShape()
{
    Triangle *newShape;

    newShape = new Triangle;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }

    makeVector(&(newShape->Normal_Vector), 0.0, 1.0, 0.0);
    makeVector(&(newShape->P1), 0.0, 0.0, 0.0);
    makeVector(&(newShape->P2), 1.0, 0.0, 0.0);
    makeVector(&(newShape->P3), 0.0, 1.0, 0.0);
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
getSmoothTriangleShape()
{
    SmoothTriangle *newShape;

    newShape = new SmoothTriangle;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }

    makeVector(&(newShape->Normal_Vector), 0.0, 1.0, 0.0);
    makeVector(&(newShape->P1), 0.0, 0.0, 0.0);
    makeVector(&(newShape->P2), 1.0, 0.0, 0.0);
    makeVector(&(newShape->P3), 0.0, 1.0, 0.0);
    makeVector(&(newShape->N1), 0.0, 1.0, 0.0);
    makeVector(&(newShape->N2), 0.0, 1.0, 0.0);
    makeVector(&(newShape->N3), 0.0, 1.0, 0.0);
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
getCsgShape()
{
    CSG *newShape;

    newShape = new CSG;
    if (newShape == nullptr) {
        Error("Out of memory. Cannot allocate shape");
    }

    newShape->Parent_Object = nullptr;
    newShape->Next_Object = nullptr;
    newShape->Shapes = nullptr;
    return (newShape);
}

CSG *
getCsgUnion()
{
    CSG *newShape;

    newShape = getCsgShape();
    newShape->methods = &CSG_Union_Methods;
    newShape->Type = CSG_UNION_TYPE;
    return (newShape);
}

CSG *
getCsgIntersection()
{
    CSG *newShape;

    newShape = getCsgShape();
    newShape->methods = &CSG_Intersection_Methods;
    newShape->Type = CSG_INTERSECTION_TYPE;
    return (newShape);
}

Viewpoint *
getViewpoint()
{
    Viewpoint *newViewpoint;

    newViewpoint = new Viewpoint;
    if (newViewpoint == nullptr) {
        Error("Out of memory. Cannot allocate viewpoint");
    }

    newViewpoint->initializeDefaults();
    return (newViewpoint);
}

RGBAColor *
getColour()
{
    RGBAColor *newColour;

    newColour = new RGBAColor;
    if (newColour == nullptr) {
        Error("Out of memory. Cannot allocate colour");
    }

    Color::makeColor(newColour, 0.0, 0.0, 0.0);
    return (newColour);
}

Vector3D *
getVector()
{
    Vector3D *newVector;

    newVector = new Vector3D;
    if (newVector == nullptr) {
        Error("Out of memory. Cannot allocate vector");
    }

    newVector->x = 0.0;
    newVector->y = 0.0;
    newVector->z = 0.0;
    return (newVector);
}

DBL *
getFloat()
{
    DBL *newFloat;

    newFloat = new DBL;
    if (newFloat == nullptr) {
        Error("Out of memory. Cannot allocate float");
    }

    *newFloat = 0.0;
    return (newFloat);
}

/* Parse a float.  Doesn't handle exponentiation. */
DBL
parseFloat()
{
    DBL localFloat = 0.0;
    CONSTANT constantId;
    register int negative;
    register int signParsed;

    negative = FALSE;
    signParsed = FALSE;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == FLOAT_CONSTANT) {
            localFloat = *((DBL *)constants[(int)constantId].Constant_Data);
            if (negative) {
                localFloat *= -1.0;
            }
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

    case PLUS_TOKEN: if (signParsed)
    {
        parseError(FLOAT_TOKEN);
    }
    signParsed = TRUE;
    break;

    case DASH_TOKEN:
    if (signParsed) {
        parseError(FLOAT_TOKEN);
    }
    negative = TRUE;
    signParsed = TRUE;
    break;

    case FLOAT_TOKEN:
    localFloat = globalToken.Token_Float;
    if (negative) {
        localFloat *= -1.0;
    }
    Exit_Flag = TRUE; break;

        default: parseError(FLOAT_TOKEN);
    break;
    }
        }
    }

    return (localFloat);
}

void
parseVector(Vector3D *givenVector)
{
    CONSTANT constantId;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == VECTOR_CONSTANT) {
            *givenVector =
                *((Vector3D *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        case LEFT_ANGLE_TOKEN:(givenVector->x) = parseFloat();
    (givenVector->y) = parseFloat();
    (givenVector->z) = parseFloat();
    getExpectedToken(RIGHT_ANGLE_TOKEN);
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }
}

void
parseCoeffs(int order, DBL *givenCoeffs)
{
    int i;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    for (i = 0; i < termCounts[order]; i++) {
        givenCoeffs[i] = parseFloat();
    }
    getExpectedToken(RIGHT_ANGLE_TOKEN);
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }
}

void
parseColour(RGBAColor *givenColour)
{
    CONSTANT constantId;
    Color::makeColor(givenColour, 0.0, 0.0, 0.0);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == COLOUR_CONSTANT) {
            *givenColour =
                *((RGBAColor *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    break;

    case RED_TOKEN:
    (givenColour->Red) = parseFloat();
    break;

    case GREEN_TOKEN:
    (givenColour->Green) = parseFloat();
    break;

    case BLUE_TOKEN:
    (givenColour->Blue) = parseFloat();
    break;

    case ALPHA_TOKEN:
    (givenColour->Alpha) = parseFloat();
    break;

    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    }
}

RGBAColorPalette *
parseColourMap()
{
static constexpr int MAX_ENTRIES = 20;
    RGBAColorPalette *newColourMap;
    register int i;
    register int j;

    newColourMap = new RGBAColorPalette;
    if (newColourMap == nullptr) {
        Error("Not enough memory for colour map.");
    }

    if (constructionMap == nullptr) {
        constructionMap = new RGBAColorPaletteSpan[MAX_ENTRIES];
        if (constructionMap == nullptr) {
            Error("Not enough memory for colour map.");
        }
    }

    i = 0;
    newColourMap->Transparency_Flag = FALSE;
    getExpectedToken(LEFT_CURLY_TOKEN);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_SQUARE_TOKEN:
    constructionMap[i].start = parseFloat();
    constructionMap[i].end = parseFloat();

    getExpectedToken(COLOUR_TOKEN);
    parseColour(&(constructionMap[i].Start_Colour));
    if (constructionMap[i].Start_Colour.Alpha != 0.0) {
        newColourMap->Transparency_Flag = TRUE;
    }

    getExpectedToken(COLOUR_TOKEN);
    parseColour(&(constructionMap[i].End_Colour));
    if (constructionMap[i].End_Colour.Alpha != 0.0) {
        newColourMap->Transparency_Flag = TRUE;
    }

    i++;
    if (i > MAX_ENTRIES) {
        Error("Colour_Map too long.");
    }
    getExpectedToken(RIGHT_SQUARE_TOKEN);
    break;

    case RIGHT_CURLY_TOKEN:
    newColourMap->Number_Of_Entries = i;

    newColourMap->Colour_Map_Entries = new RGBAColorPaletteSpan[i];
    if (newColourMap == nullptr) {
        Error("Not enough memory for colour map.");
    }

    for (j = 0; j < i; j++) {
        newColourMap->Colour_Map_Entries[j] = constructionMap[j];
    }

    Exit_Flag = TRUE; break;

        default: parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return (newColourMap);
}

Texture *
copyTexture(Texture *texture)
{
    Texture *newTexture;
    Texture *localTexture;
    Texture *firstTexture;
    Texture *previousTexture;

    previousTexture = firstTexture = nullptr;

    for (localTexture = texture; localTexture != nullptr;
         localTexture = localTexture->Next_Texture) {
        newTexture = getTexture();
        *newTexture = *localTexture;

        if (firstTexture == nullptr) {
            firstTexture = newTexture;
        }

        if (previousTexture != nullptr) {
            previousTexture->Next_Texture = newTexture;
        }

        if (newTexture->Texture_Transformation) {
            newTexture->Texture_Transformation = new Transformation;
            if (newTexture->Texture_Transformation == nullptr) {
                Error("Out of memory. Cannot allocate texture transformation");
            }
            *newTexture->Texture_Transformation =
                *localTexture->Texture_Transformation;
        }
        newTexture->Constant_Flag = FALSE;
        previousTexture = newTexture;
    }
    return (firstTexture);
}

Texture *
parseTexture()
{
    Vector3D localVector;
    CONSTANT constantId;
    Texture *texture;
    Texture *localTexture;
    Texture *firstTexture;
    Texture *tempTexture;
    int reg;

    texture = Default_Texture;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == TEXTURE_CONSTANT) {
            texture = ((Texture *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    break;

    case FLOAT_TOKEN:
    ungetToken();
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Randomness = parseFloat();
    break;

    case ONCE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Once_Flag = TRUE;
    break;

    case TURBULENCE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Turbulence = parseFloat();
    break;

    case OCTAVES_TOKEN: /* dmf 02/05 for turb */
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Octaves = (int)parseFloat();
    if (texture->Octaves < 1) {
        texture->Octaves = 6;
    }
    if (texture->Octaves > 10) { /* Avoid DOMAIN errors */
        texture->Octaves = 10;
    }
    break;

    case BOZO_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = BOZO_TEXTURE;
    break;

    case MORTAR_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Mortar = parseFloat();
    if (texture->Mortar < 0) {
        texture->Mortar = 0.2;
    }
    break;

    case BRICK_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = BRICK_TEXTURE;
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case COLOUR_TOKEN:
    texture->Colour1 = getColour();
    texture->Colour2 = getColour();
    parseColour(texture->Colour1);
    getExpectedToken(COLOUR_TOKEN);
    parseColour(texture->Colour2);
    break;

    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    } break;

    case CHECKER_TOKEN: if (texture->Constant_Flag)
    {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = CHECKER_TEXTURE;
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case COLOUR_TOKEN:
    texture->Colour1 = getColour();
    texture->Colour2 = getColour();
    parseColour(texture->Colour1);
    getExpectedToken(COLOUR_TOKEN);
    parseColour(texture->Colour2);
    break;

    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    } break;

    case CHECKER_TEXTURE_TOKEN: if (texture->Constant_Flag)
    {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = CHECKER_TEXTURE_TEXTURE;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }
    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = (Texture *)texture->Colour1;
        texture->Colour1 = (RGBAColor *)localTexture;
    }
    break;
    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    }

        getExpectedToken(TILE2_TOKEN);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = (Texture *)texture->Colour2;
        texture->Colour2 = (RGBAColor *)localTexture;
    }
    break;
    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    } getExpectedToken(RIGHT_CURLY_TOKEN);
    break;

    case MARBLE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = MARBLE_TEXTURE;
    break;

    case WOOD_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = WOOD_TEXTURE;
    break;

    case SPOTTED_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = SPOTTED_TEXTURE;
    break;

    case AGATE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = AGATE_TEXTURE;
    break;

    case GRANITE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = GRANITE_TEXTURE;
    break;

    case GRADIENT_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = GRADIENT_TEXTURE;
    parseVector(&(texture->Texture_Gradient));
    break;

    case AMBIENT_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Ambient) = parseFloat();
    break;

    case BRILLIANCE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Brilliance) = parseFloat();
    break;

    case ROUGHNESS_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Roughness) = parseFloat();
    /* No training wheels */
    /* if (texture -> Object_Roughness > 1.0)
        texture -> Object_Roughness = 1.0;
    if (texture -> Object_Roughness < 0.001)
        texture -> Object_Roughness = 0.001; */
    break;

    case PHONGSIZE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_PhongSize) = parseFloat();
    /* No training wheels */
    /*if (texture -> Object_PhongSize < 1.0)
        texture -> Object_PhongSize = 1.0;
    if (texture -> Object_PhongSize > 100)
        texture -> Object_PhongSize = 100; */
    break;

    case DIFFUSE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Diffuse) = parseFloat();
    break;

    case SPECULAR_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Specular) = parseFloat();
    break;

    case PHONG_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Phong) = parseFloat();
    break;

    case METALLIC_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Metallic_Flag = TRUE;
    break;

    case IOR_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Index_Of_Refraction) = parseFloat();
    break;

    case REFRACTION_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Refraction) = parseFloat();
    break;

    case TRANSMIT_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Transmit) = parseFloat();
    break;

    case REFLECTION_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    (texture->Object_Reflection) = parseFloat();
    break;

    case IMAGEMAP_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = IMAGEMAP_TEXTURE;
    texture->Image = new RGBAImage;
    if (texture->Image == nullptr) {
        Error("Out of memory. Cannot allocate imagemap texture");
    }
    makeVector(&texture->Image->Image_Gradient, 1.0, -1.0, 0.0);
    texture->Image->Map_Type = PLANAR_MAP;
    texture->Image->Interpolation_Type = NO_INTERPOLATION;
    texture->Image->Once_Flag = FALSE;
    texture->Image->Use_Colour_Flag = TRUE;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
    ungetToken();(texture->Image->Map_Type) = (int)parseFloat();
    break;

    case LEFT_ANGLE_TOKEN:
    ungetToken();
    parseVector(&(texture->Image->Image_Gradient));
    break;

    case IFF_TOKEN:
    getExpectedToken(STRING_TOKEN);
    readIffImage(texture->Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case GIF_TOKEN: getExpectedToken(STRING_TOKEN);
    readGifImage(texture->Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case TGA_TOKEN: getExpectedToken(STRING_TOKEN);
    readTargaImage(texture->Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case DUMP_TOKEN: getExpectedToken(STRING_TOKEN);
    readDumpImage(texture->Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        default: parseError(GIF_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case ONCE_TOKEN:
    texture->Image->Once_Flag = TRUE;
    break;

    case INTERPOLATE_TOKEN:
    texture->Image->Interpolation_Type = (int)parseFloat();
    break;

    case MAPTYPE_TOKEN:
    (texture->Image->Map_Type) = (int)parseFloat();
    break;

    case USE_COLOUR_TOKEN:
    texture->Image->Use_Colour_Flag = TRUE;
    break;

    case USE_INDEX_TOKEN:
    texture->Image->Use_Colour_Flag = FALSE;
    break;

    case ALPHA_TOKEN:
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case FLOAT_TOKEN:
    reg = (int)(globalToken.Token_Float + 0.01);
    if (texture->Image->Colour_Map == nullptr) {
        Error("Can't apply ALPHA to a non colour-mapped image\n");
    }

    if ((reg < 0) || (reg >= texture->Image->Colour_Map_Size)) {
        Error("ALPHA colour register value out of range.\n");
    }

    texture->Image->Colour_Map[reg].Alpha =
        (unsigned short)(255.0 * parseFloat());
    Exit_Flag = TRUE; break;

    case ALL_TOKEN:
    {
        DBL alpha;
        alpha = parseFloat();

        for (reg = 0; reg < texture->Image->Colour_Map_Size; reg++) {
            texture->Image->Colour_Map[reg].Alpha =
                (unsigned short)(alpha * 255.0);
        }
        Exit_Flag = TRUE;
    }

    break;
    }
        }
    }
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
    break;

    case WAVES_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = WAVES;
    texture->Bump_Amount = parseFloat();
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case PHASE_TOKEN:
    texture->Phase = parseFloat();
    Exit_Flag = TRUE; break;

        default: ungetToken(); Exit_Flag = TRUE; break; }
        }
    } break;

        case FREQUENCY_TOKEN: if (texture->Constant_Flag)
    {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Frequency = parseFloat();
    break;

    case PHASE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Phase = parseFloat();
    break;

    case RIPPLES_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = RIPPLES;
    texture->Bump_Amount = parseFloat();
    break;

    case WRINKLES_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = WRINKLES;
    texture->Bump_Amount = parseFloat();
    break;

    case BUMPS_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = BUMPS;
    texture->Bump_Amount = parseFloat();
    break;

    case DENTS_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = DENTS;
    texture->Bump_Amount = parseFloat();
    break;

    case TRANSLATE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    parseVector(&localVector);
    translateTexture(&texture, &localVector);
    break;

    case ROTATE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    parseVector(&localVector);
    rotateTexture(&texture, &localVector);
    break;

    case SCALE_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    parseVector(&localVector);
    scaleTexture(&texture, &localVector);
    break;

    case COLOUR_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Colour1 = getColour();
    parseColour(texture->Colour1);
    texture->Texture_Number = COLOUR_TEXTURE;
    break;

    case COLOUR_MAP_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Colour_Map = parseColourMap();
    break;

    case ONION_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = ONION_TEXTURE;
    break;

    case LEOPARD_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = LEOPARD_TEXTURE;
    break;

    /* New Texture Parsing - Cdw */
    case PAINTED1_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = PAINTED1_TEXTURE;
    break;

    case PAINTED2_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = PAINTED2_TEXTURE;
    break;

    case PAINTED3_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = PAINTED3_TEXTURE;
    break;

    case BUMPY1_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = BUMPY1;
    texture->Bump_Amount = parseFloat();
    break;

    case BUMPY2_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = BUMPY2;
    texture->Bump_Amount = parseFloat();
    break;

    case BUMPY3_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = BUMPY3;
    texture->Bump_Amount = parseFloat();
    break;

    case BUMPMAP_TOKEN:
    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Bump_Number = BUMPMAP;
    texture->Bump_Image = new RGBAImage;
    if (texture->Bump_Image == nullptr) {
        Error("Out of memory. Cannot allocate bumpmap texture");
    }
    makeVector(&texture->Bump_Image->Image_Gradient, 1.0, -1.0, 0.0);
    texture->Bump_Image->Map_Type = PLANAR_MAP;
    texture->Bump_Image->Interpolation_Type = NO_INTERPOLATION;
    texture->Bump_Image->Once_Flag = FALSE;
    texture->Bump_Image->Use_Colour_Flag = TRUE;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
    ungetToken();(texture->Bump_Image->Map_Type) = (int)parseFloat();
    break;

    case LEFT_ANGLE_TOKEN:
    ungetToken();
    parseVector(&(texture->Bump_Image->Image_Gradient));
    break;

    case IFF_TOKEN:
    getExpectedToken(STRING_TOKEN);
    readIffImage(texture->Bump_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case GIF_TOKEN: getExpectedToken(STRING_TOKEN);
    readGifImage(texture->Bump_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case TGA_TOKEN: getExpectedToken(STRING_TOKEN);
    readTargaImage(texture->Bump_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case DUMP_TOKEN: getExpectedToken(STRING_TOKEN);
    readDumpImage(texture->Bump_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        default: parseError(GIF_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case ONCE_TOKEN:
    texture->Bump_Image->Once_Flag = TRUE;
    break;

    case MAPTYPE_TOKEN:
    (texture->Bump_Image->Map_Type) = (int)parseFloat();
    break;

    case INTERPOLATE_TOKEN:
    texture->Bump_Image->Interpolation_Type = (int)parseFloat();
    break;

    case BUMPSIZE_TOKEN:
    texture->Bump_Amount = parseFloat();
    break;

    case USE_COLOUR_TOKEN:
    texture->Bump_Image->Use_Colour_Flag = TRUE;
    break;
    case USE_INDEX_TOKEN:
    texture->Bump_Image->Use_Colour_Flag = FALSE;
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break; default: parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
    break;

    case MATERIAL_MAP_TOKEN:

    if (texture->Constant_Flag) {
        texture = copyTexture(texture);
        texture->Constant_Flag = FALSE;
    }
    texture->Texture_Number = MATERIAL_MAP_TEXTURE;
    texture->Material_Image = new RGBAImage;
    if (texture->Material_Image == nullptr) {
        Error("Out of memory. Cannot allocate material map texture");
    }
    makeVector(&texture->Texture_Gradient, 1.0, -1.0, 0.0);
    texture->Material_Image->Map_Type = PLANAR_MAP;
    texture->Material_Image->Interpolation_Type = NO_INTERPOLATION;
    texture->Material_Image->Once_Flag = FALSE;
    texture->Material_Image->Use_Colour_Flag = FALSE;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
    ungetToken();(texture->Image->Map_Type) = (int)parseFloat();
    break;

    case LEFT_ANGLE_TOKEN:
    ungetToken();
    parseVector(&(texture->Material_Image->Image_Gradient));
    break;

    case IFF_TOKEN:
    getExpectedToken(STRING_TOKEN);
    readIffImage(texture->Material_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case GIF_TOKEN: getExpectedToken(STRING_TOKEN);
    readGifImage(texture->Material_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case TGA_TOKEN: getExpectedToken(STRING_TOKEN);
    readTargaImage(texture->Material_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        case DUMP_TOKEN: getExpectedToken(STRING_TOKEN);
    readDumpImage(texture->Material_Image, globalToken.Token_String);
    Exit_Flag = TRUE; break;

        default: parseError(GIF_TOKEN);
    break;
    }
        }
    }

    /* remember where the First_Texture is */
    firstTexture = texture;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {

    case MAPTYPE_TOKEN:
    (texture->Material_Image->Map_Type) = (int)parseFloat();
    break;

    case INTERPOLATE_TOKEN:
    texture->Material_Image->Interpolation_Type = (int)parseFloat();
    break;

    case ONCE_TOKEN:
    texture->Material_Image->Once_Flag = TRUE;
    break;

    case TEXTURE_TOKEN:
    {
        texture->Next_Material = parseTexture();
        firstTexture->Number_Of_Materials++;
        texture = texture->Next_Material;
    }

    break;

    case RIGHT_CURLY_TOKEN:
    {
        texture->Next_Material = nullptr;
        texture = firstTexture;
        Exit_Flag = TRUE;
    }
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
    return (texture);
}

Geometry *
parseLightSource()
{
    Light *localShape = nullptr;
    Vector3D localVector;
    CONSTANT constantId;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    ungetToken();
    localShape = getLightSourceShape();
    parseVector(&(localShape->Center));
    localShape->Shape_Colour = getColour();
    Color::makeColor(localShape->Shape_Colour, 1.0, 1.0, 1.0);
    localShape->Shape_Colour->Alpha = 0.0;
    getExpectedToken(COLOUR_TOKEN);
    parseColour(localShape->Shape_Colour);
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == LIGHT_SOURCE_CONSTANT) {
            localShape = (Light *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    /* Point that the spot is pointed at */
    case POINT_AT_TOKEN:
    parseVector(&(localShape->Points_At));
    break;

    case TIGHTNESS_TOKEN:
    localShape->Coeff = parseFloat();
    break;

    case RADIUS_TOKEN:
    localShape->Radius = cos(parseFloat() * M_PI / 180.0);
    break;

    case COLOUR_TOKEN:
    parseColour(localShape->Shape_Colour);
    break;

    case FALLOFF_TOKEN:
    localShape->Falloff = cos(parseFloat() * M_PI / 180.0);
    break;

    case SPOTLIGHT_TOKEN:
    localShape->Type = SPOT_LIGHT_TYPE;
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;

    }
        }
    }
    /*  linkShapes (Local_Shape, &(Local_Shape -> Next_Light_Source),
            &(Parsing_Frame_Ptr -> Light_Sources));
*/
    return ((Geometry *)localShape);
}

Geometry *
parseSphere()
{
    Sphere *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    ungetToken();
    localShape = getSphereShape();
    parseVector(&(localShape->Center));
    localShape->Radius = parseFloat();
    localShape->Radius_Squared = localShape->Radius * localShape->Radius;
    localShape->Inverse_Radius = 1.0 / localShape->Radius;
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == SPHERE_CONSTANT) {
            localShape = (Sphere *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parsePlane()
{
    InfinitePlane *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    ungetToken();
    localShape = getPlaneShape();
    parseVector(&(localShape->Normal_Vector));
    localShape->Distance = parseFloat();
    localShape->Distance *= -1.0;
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == PLANE_CONSTANT) {
            localShape = (InfinitePlane *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }
    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }
        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parseHeightField()
{
    HeightField *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    RGBAImage *image = nullptr;
    int imageType = 0;

    localShape = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) { /* This should be modified to include other image types - CdW */
        case GIF_TOKEN: imageType = GIF;
    localShape = getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        Error("Out of memory. Cannot allocate space for Height Field (1st "
              "message).");
    }
    getExpectedToken(STRING_TOKEN);
    readGifImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    makeVector(
        &localVector, 1.0 / (image->width), 1.0 / 256.0, 1.0 / (image->height));
    getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

        case POT_TOKEN: imageType = POT;
    localShape = getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        Error("Out of memory. Cannot allocate space for Height Field (1st "
              "message).");
    }
    getExpectedToken(STRING_TOKEN);
    readGifImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width / 2.0 - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    makeVector(
        &localVector, 2.0 / image->width, 1.0 / 256.0, 1.0 / image->height);
    getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

        case TGA_TOKEN: imageType = TGA;
    localShape = getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        Error("Cannot allocate space for Height Field (1st message).");
    }
    getExpectedToken(STRING_TOKEN);
    readTargaImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    makeVector(
        &localVector, 1.0 / image->width, 1.0 / 256.0, 1.0 / image->height);
    getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == HEIGHT_FIELD_CONSTANT) {
            localShape = (HeightField *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(GIF_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case WATER_LEVEL_TOKEN: localShape->bounding_box->bounds[0]
            .y = parseFloat();
    break;

    case TRANSLATE_TOKEN:
    parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    {
        Texture *tempTexture;

        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    findHfMinMax(localShape, image, imageType);
    return ((Geometry *)localShape);
}

Geometry *
parseTriangle()
{
    Triangle *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    ungetToken();
    localShape = getTriangleShape();
    parseVector(&localShape->P1);
    parseVector(&localShape->P2);
    parseVector(&localShape->P3);
    if (!computeTriangle(localShape)) {
        fprintf(stderr, "Degenerate triangle on line %d.  Please remove.\n",
            globalToken.Token_Line_No);
        degenerateTriangles = TRUE;
    }
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == TRIANGLE_CONSTANT) {
            localShape = (Triangle *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }
    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parseSmoothTriangle()
{
    SmoothTriangle *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    ungetToken();
    localShape = (SmoothTriangle *)getSmoothTriangleShape();
    parseVector(&localShape->P1);
    parseVector(&localShape->N1);
    VNormalize(localShape->N1, localShape->N1);
    parseVector(&localShape->P2);
    parseVector(&localShape->N2);
    VNormalize(localShape->N2, localShape->N2);
    parseVector(&localShape->P3);
    parseVector(&localShape->N3);
    VNormalize(localShape->N3, localShape->N3);
    if (!computeTriangle((Triangle *)localShape))
    {
        fprintf(stderr, "Degenerate triangle on line %d.  Please remove.\n",
            globalToken.Token_Line_No);
        degenerateTriangles = TRUE;
    }
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type ==
            SMOOTH_TRIANGLE_CONSTANT) {
            localShape = (SmoothTriangle *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parseQuadric()
{
    Quadric *localShape;
    Vector3D localVector;
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    ungetToken();
    localShape = getQuadricShape();
    parseVector(&(localShape->Object_2_Terms));
    parseVector(&(localShape->Object_Mixed_Terms));
    parseVector(&(localShape->Object_Terms));
    (localShape->Object_Constant) = parseFloat();
    localShape->Non_Zero_Square_Term =
        !((localShape->Object_2_Terms.x == 0.0) &&
            (localShape->Object_2_Terms.y == 0.0) &&
            (localShape->Object_2_Terms.z == 0.0) &&
            (localShape->Object_Mixed_Terms.x == 0.0) &&
            (localShape->Object_Mixed_Terms.y == 0.0) &&
            (localShape->Object_Mixed_Terms.z == 0.0));
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == QUADRIC_CONSTANT) {
            localShape = (Quadric *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }
    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parsePoly(int knownOrder)
{
    Poly *localShape;
    Vector3D localVector;
    CONSTANT constantId;
    int order;
    Texture *localTexture;

    if (knownOrder > 0) {
        localShape = getPolyShape(knownOrder);
    } else {
        localShape = nullptr;
    }

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
    ungetToken();
    if (localShape != nullptr) {
        Error("The order of a polynomial may not be specified twice");
    }
    order = (int)parseFloat();
    if (order < 2 || order > MAX_ORDER) {
        Error("Order of Poly is out of range");
    }
    localShape = getPolyShape(order);
    break;

    case LEFT_ANGLE_TOKEN:
    ungetToken();
    if (localShape == nullptr) {
        printf("Need the order of the Poly");
    }
    parseCoeffs(localShape->Order, &(localShape->Coeffs[0]));
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == POLY_CONSTANT) {
            localShape = (Poly *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case STURM_TOKEN: localShape->Sturm_Flag = 1;
    break;

    case TRANSLATE_TOKEN:
    parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    Link((SimpleBody *)localTexture, (SimpleBody **)&localTexture->Next_Texture,
        (SimpleBody **)&localShape->Shape_Texture);
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parseBicubicPatch()
{
    BicubicPatch *localShape = nullptr;
    Vector3D localVector;
    CONSTANT constantId;
    Texture *localTexture;
    int i;
    int j;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
    ungetToken();
    localShape = getBicubicPatchShape();
    localShape->Patch_Type = (int)parseFloat();
    if (localShape->Patch_Type == 2 || localShape->Patch_Type == 3) {
        localShape->Flatness_Value = parseFloat();
    } else {
        localShape->Flatness_Value = 0.1;
    }
    localShape->U_Steps = (int)parseFloat();
    localShape->V_Steps = (int)parseFloat();
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            parseVector(&(localShape->Control_Points[i][j]));
        }
    }
    precomputePatchValues(localShape); /* interpolated mesh coords */
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type ==
            BICUBIC_PATCH_CONSTANT) {
            localShape = (BicubicPatch *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    Link((SimpleBody *)localTexture, (SimpleBody **)&localTexture->Next_Texture,
        (SimpleBody **)&localShape->Shape_Texture);
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parseBox()
{
    Box *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_CURLY_TOKEN:
    Exit_Flag = TRUE; break; default: parseError(LEFT_CURLY_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    ungetToken();
    localShape = getBoxShape();
    parseVector(&(localShape->bounds[0]));
    parseVector(&(localShape->bounds[1]));
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == BOX_CONSTANT) {
            localShape = (Box *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(LEFT_ANGLE_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }
    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

Geometry *
parseBlob()
{
    Blob *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;
    DBL threshold;
    int npoints;
    BlobList *blobComponents;
    BlobList *blobComponent;

    localShape = nullptr;
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LEFT_CURLY_TOKEN:
    Exit_Flag = TRUE; break; default: parseError(LEFT_CURLY_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case THRESHOLD_TOKEN:
    case COMPONENT_TOKEN:
    ungetToken();
    localShape = getBlobShape();
    blobComponents = nullptr;
    npoints = 0;
    threshold = 1.0;

    /* Here is where we get the blob coefficients */
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case THRESHOLD_TOKEN:
    threshold = parseFloat();
    break;

    case COMPONENT_TOKEN:
    blobComponent = new BlobList;
    if (blobComponent == nullptr) {
        Error("Out of Memory! Cannot allocate blob component");
    }
    blobComponent->elem.coeffs[2] = parseFloat();
    blobComponent->elem.radius2 = parseFloat();
    parseVector(&blobComponent->elem.pos);
    blobComponent->next = blobComponents;
    blobComponents = blobComponent;
    npoints++;
    break;

    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    }

        /* Finally, process the information */
        MakeBlob(
            (SimpleBody *)localShape, threshold, blobComponents, npoints, 0);
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == BLOB_CONSTANT) {
            localShape = (Blob *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    }
    else
    {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: parseError(FLOAT_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case STURM_TOKEN: localShape->Sturm_Flag = 1;
    break;

    case TRANSLATE_TOKEN:
    parseVector(&localVector);
    Translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }
    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = getColour();
    parseColour(localShape->Shape_Colour);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((Geometry *)localShape);
}

CSG *
parseCsg(int type, SimpleBody *parentObject)
{
    CSG *container = nullptr;
    Geometry *localShape;
    Vector3D localVector;
    CONSTANT constantId;
    int firstShapeParsed = FALSE;

    if (type == CSG_UNION_TYPE) {
        container = getCsgUnion();

    } else if ((type == CSG_INTERSECTION_TYPE) ||
               (type == CSG_DIFFERENCE_TYPE)) {
        container = getCsgIntersection();
    }

    container->Parent_Object = parentObject;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if ((constants[(int)constantId].Constant_Type ==
                CSG_INTERSECTION_CONSTANT) ||
            (constants[(int)constantId].Constant_Type == CSG_UNION_CONSTANT) ||
            (constants[(int)constantId].Constant_Type ==
                CSG_DIFFERENCE_CONSTANT)) {
            delete container;
            container = (CSG *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
            setCsgParents(container, parentObject);
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    break;

    case LIGHT_SOURCE_TOKEN:
    localShape = parseLightSource();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case SPHERE_TOKEN:
    localShape = parseSphere();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case PLANE_TOKEN:
    localShape = parsePlane();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case TRIANGLE_TOKEN:
    localShape = parseTriangle();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case SMOOTH_TRIANGLE_TOKEN:
    localShape = parseSmoothTriangle();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case QUADRIC_TOKEN:
    localShape = parseQuadric();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case HEIGHT_FIELD_TOKEN:
    localShape = parseHeightField();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case CUBIC_TOKEN:
    localShape = parsePoly(3);
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case QUARTIC_TOKEN:
    localShape = parsePoly(4);
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case POLY_TOKEN:
    localShape = parsePoly(0);
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case BOX_TOKEN:
    localShape = parseBox();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case BLOB_TOKEN:
    localShape = parseBlob();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case BICUBIC_PATCH_TOKEN:
    localShape = parseBicubicPatch();
    localShape->Parent_Object = parentObject;
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case UNION_TOKEN:
    localShape = (Geometry *)parseCsg(CSG_UNION_TYPE, parentObject);
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case INTERSECTION_TOKEN:
    localShape = (Geometry *)parseCsg(CSG_INTERSECTION_TYPE, parentObject);
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    case DIFFERENCE_TOKEN:
    localShape = (Geometry *)parseCsg(CSG_DIFFERENCE_TYPE, parentObject);
    if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
        Invert((SimpleBody *)localShape);
    }
    firstShapeParsed = TRUE;
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(container->Shapes));
    break;

    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    }

        {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) { case RIGHT_CURLY_TOKEN: Exit_Flag = TRUE; break;

            case TRANSLATE_TOKEN: parseVector(&localVector);
    Translate((SimpleBody *)container, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)container, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)container, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)container);
    break;

    default:
    if (type == CSG_UNION_TYPE) {
        parseError(RIGHT_CURLY_TOKEN);
    } else if (type == CSG_INTERSECTION_TYPE) {
        parseError(RIGHT_CURLY_TOKEN);
    } else {
        parseError(RIGHT_CURLY_TOKEN);
    }
    break;
    }
        }
    }

    return ((CSG *)container);
}

Geometry *
parseShape(SimpleBody *object)
{
    Geometry *localShape = nullptr;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case LIGHT_SOURCE_TOKEN:
    localShape = parseLightSource();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case SPHERE_TOKEN: localShape = parseSphere();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case PLANE_TOKEN: localShape = parsePlane();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case TRIANGLE_TOKEN: localShape = parseTriangle();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case SMOOTH_TRIANGLE_TOKEN: localShape = parseSmoothTriangle();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case QUADRIC_TOKEN: localShape = parseQuadric();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case HEIGHT_FIELD_TOKEN: localShape = parseHeightField();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case CUBIC_TOKEN: localShape = parsePoly(3);
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case QUARTIC_TOKEN: localShape = parsePoly(4);
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case POLY_TOKEN: localShape = parsePoly(0);
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case BOX_TOKEN: localShape = parseBox();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case BLOB_TOKEN: localShape = parseBlob();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case BICUBIC_PATCH_TOKEN: localShape = parseBicubicPatch();
    localShape->Parent_Object = object;
    Exit_Flag = TRUE; break;

        case UNION_TOKEN: localShape =
            (Geometry *)parseCsg(CSG_UNION_TYPE, object);
    Exit_Flag = TRUE; break;

        case INTERSECTION_TOKEN: localShape =
            (Geometry *)parseCsg(CSG_INTERSECTION_TYPE, object);
    Exit_Flag = TRUE; break;

        case DIFFERENCE_TOKEN: localShape =
            (Geometry *)parseCsg(CSG_DIFFERENCE_TYPE, object);
    Exit_Flag = TRUE; break;

        default: parseError(QUADRIC_TOKEN);
    break;
    }
        }
    }
    return (localShape);
}

SimpleBody *
parseObject()
{
    SimpleBody *object;
    Geometry *localShape;
    Vector3D localVector;
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;

    object = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == OBJECT_CONSTANT) {
            object = (SimpleBody *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    Exit_Flag = TRUE; break;

    case SPHERE_TOKEN:
    case QUADRIC_TOKEN:
    case QUARTIC_TOKEN:
    case UNION_TOKEN:
    case INTERSECTION_TOKEN:
    case DIFFERENCE_TOKEN:
        case TRIANGLE_TOKEN:
    case SMOOTH_TRIANGLE_TOKEN:
    case PLANE_TOKEN:
    case CUBIC_TOKEN:
    case POLY_TOKEN:
    case BICUBIC_PATCH_TOKEN:
            case HEIGHT_FIELD_TOKEN:
    case LIGHT_SOURCE_TOKEN:
    case BOX_TOKEN:
    case BLOB_TOKEN:
                ungetToken(); if (object == nullptr)
    {
        object = getObject();
    }

    localShape = parseShape(object);
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(object->Shape));
    Exit_Flag = TRUE; break;

        default: parseError(SHAPE_TOKEN);
    Exit_Flag = TRUE; break; }
        }
    }

        {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) { case BOUNDED_TOKEN:

            getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ungetToken(); localShape = parseShape(object);
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(object->Bounding_Shapes));
    break;
    }
        }
    }
    break;

    case CLIPPED_TOKEN:

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ungetToken(); localShape = parseShape(object);
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(object->Clipping_Shapes));
    break;
    }
        }
    }
    break;

    case COLOUR_TOKEN:
    object->Object_Colour = getColour();
    parseColour(object->Object_Colour);
    break;

    case TEXTURE_TOKEN:
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    if (object->Object_Texture == Default_Texture) {
        object->Object_Texture = localTexture;
    } else {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = object->Object_Texture;
        object->Object_Texture = localTexture;
    }
    break;

    case NO_SHADOW_TOKEN:
    object->No_Shadow_Flag = TRUE;
    break;

    case LIGHT_SOURCE_TOKEN:
    Error("Light source must be defined using new syntax");
    break;

    case TRANSLATE_TOKEN:
    parseVector(&localVector);
    Translate(object, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate(object, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale(object, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert(object);
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: parseError(RIGHT_CURLY_TOKEN);
    break;

    }
        }
    }

    return (object);
}

SimpleBody *
parseComposite()
{
    Composite *localComposite;
    SimpleBody *localObject;
    Geometry *localShape;
    CONSTANT constantId;
    Vector3D localVector;

    localComposite = nullptr;

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == COMPOSITE_CONSTANT) {
            localComposite = (Composite *)Copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    break;

    case COMPOSITE_TOKEN:
    if (localComposite == nullptr) {
        localComposite = getCompositeObject();
    }

    localObject = parseComposite();
    Link((SimpleBody *)localObject, (SimpleBody **)&(localObject->Next_Object),
        (SimpleBody **)&(localComposite->Objects));
    break;

    case OBJECT_TOKEN:
    if (localComposite == nullptr) {
        localComposite = getCompositeObject();
    }
    localObject = parseObject();
    Link(localObject, &(localObject->Next_Object), &(localComposite->Objects));
    break;

    case RIGHT_CURLY_TOKEN:
    ungetToken();
    if (localComposite == nullptr) {
        localComposite = getCompositeObject();
    }
    Exit_Flag = TRUE; break;

        default: ungetToken(); Exit_Flag = TRUE; break; }
        }
    }

            {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) { case RIGHT_CURLY_TOKEN: Exit_Flag = TRUE; break;

                case BOUNDED_TOKEN:

                    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ungetToken(); localShape = parseShape((SimpleBody *)localComposite);
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(localComposite->Bounding_Shapes));
    break;
    }
        }
    }
    break;

    case CLIPPED_TOKEN:

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ungetToken(); localShape = parseShape((SimpleBody *)localComposite);
    Link((SimpleBody *)localShape, (SimpleBody **)&(localShape->Next_Object),
        (SimpleBody **)&(localComposite->Clipping_Shapes));
    break;
    }
        }
    }
    break;

    case TRANSLATE_TOKEN:
    parseVector(&localVector);
    Translate((SimpleBody *)localComposite, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)localComposite, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)localComposite, &localVector);
    break;

    case INVERSE_TOKEN:
    Invert((SimpleBody *)localComposite);
    break;

    default:
    parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    return ((SimpleBody *)localComposite);
}

void
parseFog()
{
    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case COLOUR_TOKEN:
    parseColour(&parsingFramePtr->Fog_Colour);
    break;

    case FLOAT_TOKEN:
    parsingFramePtr->Fog_Distance = globalToken.Token_Float;
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
}

void
parseFrame()
{
    SimpleBody *localObject;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case FOG_TOKEN:
    parseFog();
    break;

    case DEFAULT_TOKEN:
    getExpectedToken(LEFT_CURLY_TOKEN);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case TEXTURE_TOKEN:
    Default_Texture->Constant_Flag = FALSE;
    Default_Texture = parseTexture();
    Default_Texture->Constant_Flag = TRUE;
    break;
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
    break;

    case MAX_TRACE_LEVEL_TOKEN:
    maxTraceLevel = parseFloat();
    break;

    case OBJECT_TOKEN:
    localObject = parseObject();
    Link(localObject, &(localObject->Next_Object), &(parsingFramePtr->Objects));

    /* light sources are now linked in object parsing */
    /* if (Local_Object -> Light_Source_Flag)
    Link(Local_Object, &(Local_Object -> Next_Light_Source),
            &(Parsing_Frame_Ptr -> Light_Sources)); */
    break;

    case COMPOSITE_TOKEN:
    localObject = parseComposite();
    Link(localObject, &(localObject->Next_Object), &(parsingFramePtr->Objects));
    /*        addCompositeLightSource ((Composite *)Local_Object);*/
    break;

    case VIEW_POINT_TOKEN:
    parseViewpoint(&(parsingFramePtr->View_Point));
    break;

    case DECLARE_TOKEN:
    parseDeclare();
    break;

    case END_OF_FILE_TOKEN:
    Exit_Flag = TRUE; break;

        default: parseError(OBJECT_TOKEN);
    break;
    }
        }
    }
}

void
parseViewpoint(Viewpoint *givenVp)
{
    CONSTANT constantId;
    Vector3D localVector;
    Vector3D tempVector;
    DBL directionLength, upLength, rightLength, handedness;

    givenVp->initializeDefaults();

    getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == VIEW_POINT_CONSTANT) {
            *givenVp = *((Viewpoint *)constants[(int)constantId].Constant_Data);
        } else {
            typeError();
        }
    } else {
        Undeclared();
    }
    break;

    case LOCATION_TOKEN:
    parseVector(&(givenVp->Location));
    break;

    case DIRECTION_TOKEN:
    parseVector(&(givenVp->Direction));
    break;

    case UP_TOKEN:
    parseVector(&(givenVp->Up));
    break;

    case RIGHT_TOKEN:
    parseVector(&(givenVp->Right));
    break;

    case SKY_TOKEN:
    parseVector(&(givenVp->Sky));
    break;

    case LOOK_AT_TOKEN:
    VLength(directionLength, givenVp->Direction);
    VLength(upLength, givenVp->Up);
    VLength(rightLength, givenVp->Right);
    VCross(tempVector, givenVp->Direction, givenVp->Up);
    VDot(handedness, tempVector, givenVp->Right);
    parseVector(&givenVp->Direction);

    VSub(givenVp->Direction, givenVp->Direction, givenVp->Location);
    VNormalize(givenVp->Direction, givenVp->Direction);
    VCross(givenVp->Right, givenVp->Direction, givenVp->Sky);
    VNormalize(givenVp->Right, givenVp->Right);
    VCross(givenVp->Up, givenVp->Right, givenVp->Direction);
    VScale(givenVp->Direction, givenVp->Direction, directionLength);
    if (handedness >= 0.0) {
        VScale(givenVp->Right, givenVp->Right, rightLength);
    } else {
        VScale(givenVp->Right, givenVp->Right, -rightLength);
    }

    VScale(givenVp->Up, givenVp->Up, upLength);
    break;

    case TRANSLATE_TOKEN:
    parseVector(&localVector);
    Translate((SimpleBody *)givenVp, &localVector);
    break;

    case ROTATE_TOKEN:
    parseVector(&localVector);
    Rotate((SimpleBody *)givenVp, &localVector);
    break;

    case SCALE_TOKEN:
    parseVector(&localVector);
    Scale((SimpleBody *)givenVp, &localVector);
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
}

void
parseDeclare()
{
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;
    Constant *constantPtr;

    getExpectedToken(IDENTIFIER_TOKEN);
    if ((constantId = findConstant()) == -1) {
        if (++numberOfConstants >= MAX_CONSTANTS) {
            Error("Too many constants \"declared\"");
        } else {
            constantId = numberOfConstants;
        }
    }

    constantPtr = &(constants[(int)constantId]);
    getExpectedToken(EQUALS_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case OBJECT_TOKEN:
    constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseObject();
    constantPtr->Constant_Type = OBJECT_CONSTANT;
    Exit_Flag = TRUE; break;

        case SPHERE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseSphere();
    constantPtr->Constant_Type = SPHERE_CONSTANT;
    Exit_Flag = TRUE; break;

        case PLANE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parsePlane();
    constantPtr->Constant_Type = PLANE_CONSTANT;
    Exit_Flag = TRUE; break;

        case TRIANGLE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseTriangle();
    constantPtr->Constant_Type = TRIANGLE_CONSTANT;
    Exit_Flag = TRUE; break;

        case SMOOTH_TRIANGLE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseSmoothTriangle();
    constantPtr->Constant_Type = SMOOTH_TRIANGLE_CONSTANT;
    Exit_Flag = TRUE; break;

        case QUADRIC_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseQuadric();
    constantPtr->Constant_Type = QUADRIC_CONSTANT;
    Exit_Flag = TRUE; break;

        case CUBIC_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parsePoly(3);
    constantPtr->Constant_Type = POLY_CONSTANT;
    Exit_Flag = TRUE; break;

        case QUARTIC_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parsePoly(4);
    constantPtr->Constant_Type = POLY_CONSTANT;
    Exit_Flag = TRUE; break;

        case HEIGHT_FIELD_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseHeightField();
    constantPtr->Constant_Type = HEIGHT_FIELD_CONSTANT;
    Exit_Flag = TRUE; break;

        case POLY_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parsePoly(0);
    constantPtr->Constant_Type = POLY_CONSTANT;
    Exit_Flag = TRUE; break;

        case BOX_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseBox();
    constantPtr->Constant_Type = BOX_CONSTANT;
    Exit_Flag = TRUE; break;

        case BLOB_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseBlob();
    constantPtr->Constant_Type = BLOB_CONSTANT;
    Exit_Flag = TRUE; break;

        case BICUBIC_PATCH_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseBicubicPatch();
    constantPtr->Constant_Type = BICUBIC_PATCH_CONSTANT;
    Exit_Flag = TRUE; break;

        case INTERSECTION_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data =
        (char *)parseCsg(CSG_INTERSECTION_TYPE, nullptr);
    constantPtr->Constant_Type = CSG_INTERSECTION_CONSTANT;
    Exit_Flag = TRUE; break;

        case UNION_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseCsg(CSG_UNION_TYPE, nullptr);
    constantPtr->Constant_Type = CSG_UNION_CONSTANT;
    Exit_Flag = TRUE; break;

        case DIFFERENCE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data =
        (char *)parseCsg(CSG_DIFFERENCE_TYPE, nullptr);
    constantPtr->Constant_Type = CSG_DIFFERENCE_CONSTANT;
    Exit_Flag = TRUE; break;

        case COMPOSITE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseComposite();
    constantPtr->Constant_Type = COMPOSITE_CONSTANT;
    Exit_Flag = TRUE; break;

        case TEXTURE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    localTexture = nullptr;
    constantPtr->Constant_Data = (char *)localTexture;
    constantPtr->Constant_Type = TEXTURE_CONSTANT;
    ungetToken();
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            getToken();
            switch (globalToken.Token_Id) {
    case TEXTURE_TOKEN:
    localTexture = Default_Texture;
    localTexture = parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = copyTexture(localTexture);
    }

    localTexture->Constant_Flag = TRUE;

    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = (Texture *)constantPtr->Constant_Data;
        constantPtr->Constant_Data = (char *)localTexture;
    }
    break;

    default:
    ungetToken();
    Exit_Flag = TRUE; break; }
        }
    } Exit_Flag = TRUE; break;

        case VIEW_POINT_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)getViewpoint();
    constantPtr->Constant_Type = VIEW_POINT_CONSTANT;
    parseViewpoint((Viewpoint *)constantPtr->Constant_Data);
    Exit_Flag = TRUE; break;

        case COLOUR_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)getColour();
    constantPtr->Constant_Type = COLOUR_CONSTANT;
    parseColour((RGBAColor *)constantPtr->Constant_Data);
    Exit_Flag = TRUE; break;

        case LIGHT_SOURCE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)parseLightSource();
    constantPtr->Constant_Type = LIGHT_SOURCE_CONSTANT;
    Exit_Flag = TRUE; break;

        case LEFT_ANGLE_TOKEN: ungetToken(); constantPtr->Identifier_Number =
        globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)getVector();
    constantPtr->Constant_Type = VECTOR_CONSTANT;
    parseVector((Vector3D *)constantPtr->Constant_Data);
    Exit_Flag = TRUE; break;

        case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
            ungetToken(); constantPtr->Identifier_Number =
        globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)getFloat();
    constantPtr->Constant_Type = FLOAT_CONSTANT;
    *((DBL *)constantPtr->Constant_Data) = parseFloat();
    Exit_Flag = TRUE; break;

        default: parseError(OBJECT_TOKEN);
    break;
    }
        }
    }
}

static void
linkShapes(Light *newObject, Light **field, Light **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

CONSTANT
findConstant()
{
    register int i;

    for (i = 1; i <= numberOfConstants; i++) {
        if (constants[i].Identifier_Number == globalToken.Identifier_Number) {
            return (i);
        }
    }

    return (-1);
}

char *
getTokenString(TOKEN tokenId)
{
    register int i;

    for (i = 0; i < LAST_TOKEN; i++) {
        if (globalReservedWords[i].Token_Number == tokenId) {
            return (char *)(globalReservedWords[i].Token_Name);
        }
    }
    return (char *)("");
}

void
parseError(TOKEN tokenId)
{
    char *expected;
    char *found;
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    expected = getTokenString(tokenId);
    found = getTokenString(globalToken.Token_Id);
    fprintf(stderr, "%s expected but %s found instead\n", expected, found);
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(
            statFile, "%s expected but %s found instead\n", expected, found);
        fclose(statFile);
    }

    exit(1);
}

void
typeError()
{
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    fprintf(
        stderr, "Identifier %s is the wrong type\n", globalToken.Token_String);
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(statFile, "Identifier %s is the wrong type\n",
            globalToken.Token_String);

        fclose(statFile);
    }

    exit(1);
}

void
Undeclared()
{
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    fprintf(stderr, "Undeclared identifier %s\n", globalToken.Token_String);
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(
            statFile, "Undeclared identifier %s\n", globalToken.Token_String);
        fclose(statFile);
    }

    exit(1);
}

static void
postProcessObject(SimpleBody *object)
{
    SimpleBody *temp;

    if (object->Type == COMPOSITE_TYPE) {
        for (temp = ((Composite *)object)->Objects; temp != nullptr;
             temp = temp->Next_Object) {
            postProcessObject(temp);
        }
    } else {
        postProcessShape(object->Shape);
    }
}

static void
postProcessShape(Geometry *shape)
{
    Geometry *tempShape;

    if ((shape->Type == CSG_UNION_TYPE) ||
        (shape->Type == CSG_INTERSECTION_TYPE) ||
        (shape->Type == CSG_DIFFERENCE_TYPE)) {
        for (tempShape = ((CSG *)shape)->Shapes; tempShape != nullptr;
             tempShape = tempShape->Next_Object) {
            postProcessShape(tempShape);
        }
    } else if ((shape->Type == POINT_LIGHT_TYPE) ||
               (shape->Type == SPOT_LIGHT_TYPE)) {
        linkShapes((Light *)shape, &(((Light *)shape)->Next_Light_Source),
            &(parsingFramePtr->Light_Sources));
    }
}

void
Error(const char *str)
{
    FILE *statFile;
    fprintf(stderr, "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    fprintf(stderr, "%s\n", str);

    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(statFile, "%s\n", str);
        fclose(statFile);
    }
    exit(1);
}
