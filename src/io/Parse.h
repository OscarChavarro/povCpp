#ifndef __PARSE_H__
#define __PARSE_H__

#include "common/Frame.h"
#include "geom/Csg.h"
#include "geom/ViewPnt.h"
#include "io/Raw.h"
#include "io/Targa.h"
#include "io/Tokenize.h"
#include "render/Render.h"

/* Types of constants allowed in DECLARE statement (fm. PARSE.C) */

#define OBJECT_CONSTANT 0
#define VIEW_POINT_CONSTANT 1
#define VECTOR_CONSTANT 2
#define FLOAT_CONSTANT 3
#define COLOUR_CONSTANT 4
#define QUADRIC_CONSTANT 5
#define POLY_CONSTANT 6
#define BICUBIC_PATCH_CONSTANT 7
#define SPHERE_CONSTANT 8
#define PLANE_CONSTANT 9
#define TRIANGLE_CONSTANT 10
#define SMOOTH_TRIANGLE_CONSTANT 11
#define CSG_INTERSECTION_CONSTANT 12
#define CSG_UNION_CONSTANT 13
#define CSG_DIFFERENCE_CONSTANT 14
#define COMPOSITE_CONSTANT 15
#define TEXTURE_CONSTANT 16
#define HEIGHT_FIELD_CONSTANT 17
#define BOX_CONSTANT 18
#define BLOB_CONSTANT 19
#define LIGHT_SOURCE_CONSTANT 20

class Constant {
  public:
    int Identifier_Number;
    CONSTANT Constant_Type;
    char *Constant_Data;
};

extern void Parse(Frame *Frame_Ptr);
extern void tokenInit(void);
extern void frameInit(void);
extern Texture *getTexture(void);
extern RGBAColor *getColour(void);
extern Vector3D *getVector(void);
extern DBL *getFloat(void);
extern DBL parseFloat(void);
extern void parseVector(Vector3D *Given_Vector);
extern void parseCoeffs(int order, DBL *Given_Coeffs);
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
