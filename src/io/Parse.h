#ifndef __PARSE_H__
#define __PARSE_H__

#include "common/Frame.h"
#include "geom/Csg.h"
#include "geom/ViewPnt.h"
#include "io/Raw.h"
#include "io/Targa.h"
#include "io/Tokenize.h"
#include "render/Render.h"

class Blob;
class Box;
class Composite;
class HeightField;
class InfinitePlane;
class Light;
class Poly;
class Quadric;
class SmoothTriangle;
class Sphere;
class Triangle;

/* Types of constants allowed in DECLARE statement (fm. PARSE.C) */

static constexpr int OBJECT_CONSTANT = 0;
static constexpr int VIEW_POINT_CONSTANT = 1;
static constexpr int VECTOR_CONSTANT = 2;
static constexpr int FLOAT_CONSTANT = 3;
static constexpr int COLOUR_CONSTANT = 4;
static constexpr int QUADRIC_CONSTANT = 5;
static constexpr int POLY_CONSTANT = 6;
static constexpr int BICUBIC_PATCH_CONSTANT = 7;
static constexpr int SPHERE_CONSTANT = 8;
static constexpr int PLANE_CONSTANT = 9;
static constexpr int TRIANGLE_CONSTANT = 10;
static constexpr int SMOOTH_TRIANGLE_CONSTANT = 11;
static constexpr int CSG_INTERSECTION_CONSTANT = 12;
static constexpr int CSG_UNION_CONSTANT = 13;
static constexpr int CSG_DIFFERENCE_CONSTANT = 14;
static constexpr int COMPOSITE_CONSTANT = 15;
static constexpr int TEXTURE_CONSTANT = 16;
static constexpr int HEIGHT_FIELD_CONSTANT = 17;
static constexpr int BOX_CONSTANT = 18;
static constexpr int BLOB_CONSTANT = 19;
static constexpr int LIGHT_SOURCE_CONSTANT = 20;

class Constant {
  public:
    int Identifier_Number;
    CONSTANT Constant_Type;
    char *Constant_Data;
};

class ParseFactory {
  public:
    static Composite *getCompositeObject();
    static Sphere *getSphereShape();
    static Light *getLightSourceShape();
    static Quadric *getQuadricShape();
    static Poly *getPolyShape(int order);
    static Box *getBoxShape();
    static Blob *getBlobShape();
    static HeightField *getHeightFieldShape();
    static InfinitePlane *getPlaneShape();
    static Triangle *getTriangleShape();
    static SmoothTriangle *getSmoothTriangleShape();
    static CSG *getCsgShape();
    static CSG *getCsgUnion();
    static CSG *getCsgIntersection();
    static Viewpoint *getViewpoint();
    static RGBAColor *getColour();
    static Vector3D *getVector();
    static double *getFloat();
};

extern void Parse(Frame *Frame_Ptr);
extern void tokenInit(void);
extern void frameInit(void);
extern Texture *getTexture(void);
extern RGBAColor *getColour(void);
extern Vector3D *getVector(void);
extern double *getFloat(void);
extern double parseFloat(void);
extern void parseVector(Vector3D *Given_Vector);
extern void parseCoeffs(int order, double *Given_Coeffs);
extern void parseColour(RGBAColor *Given_Colour);
extern RGBAColorPalette *parseColourMap(void);
extern Texture *parseTexture(void);
extern Geometry *parseSphere(void);
extern Geometry *parseLightSource(void);

extern Geometry *parsePlane(void);
extern Geometry *parseTriangle(void);
extern Geometry *parseSmoothTriangle(void);
extern Geometry *parseQuadric(void);
extern Geometry *parsePoly(int);
extern Geometry *parseBox(void);
extern Geometry *parseBlob(void);
extern Geometry *parseBicubicPatch(void);
extern Geometry *parseHeightField(void);
extern CSG *parseCsg(int type, SimpleBody *Parent_Object);
extern Geometry *parseShape(SimpleBody *Object);
extern SimpleBody *parseObject(void);
extern SimpleBody *parseComposite(void);
extern void parseFog(void);
extern void parseFrame(void);
extern void parseViewpoint(Viewpoint *Given_Vp);
extern void parseDeclare(void);
extern CONSTANT findConstant(void);
extern char *getTokenString(TOKEN Token_Id);
extern void parseError(TOKEN Token_Id);
extern void typeError(void);
extern void Undeclared(void);

#endif
