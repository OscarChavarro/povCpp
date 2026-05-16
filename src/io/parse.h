#ifndef __PARSE_H__
#define __PARSE_H__

#include "common/frame.h"
#include "geom/csg.h"
#include "geom/viewpnt.h"
#include "io/raw.h"
#include "io/targa.h"
#include "io/tokenize.h"
#include "render/render.h"

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
extern void Token_Init(void);
extern void Frame_Init(void);
extern Texture *Get_Texture(void);
extern RGBAColor *Get_Colour(void);
extern Vector3D *Get_Vector(void);
extern DBL *Get_Float(void);
extern DBL Parse_Float(void);
extern void Parse_Vector(Vector3D *Given_Vector);
extern void Parse_Coeffs(int order, DBL *Given_Coeffs);
extern void Parse_Colour(RGBAColor *Given_Colour);
extern RGBAColorPalette *Parse_Colour_Map(void);
extern Texture *Parse_Texture(void);
extern Geometry *Parse_Sphere(void);
extern Geometry *Parse_Light_Source(void);

extern Geometry *Parse_Plane(void);
extern Geometry *Parse_Triangle(void);
extern Geometry *Parse_Smooth_Triangle(void);
extern Geometry *Parse_Quadric(void);
extern Geometry *Parse_Poly(int);
extern Geometry *Parse_Box(void);
extern Geometry *Parse_Blob(void);
extern Geometry *Parse_Bicubic_Patch(void);
extern Geometry *Parse_Height_Field(void);
extern CSG *Parse_CSG(int type, SimpleBody *Parent_Object);
extern Geometry *Parse_Shape(SimpleBody *Object);
extern SimpleBody *Parse_Object(void);
extern SimpleBody *Parse_Composite(void);
extern void Parse_Fog(void);
extern void Parse_Frame(void);
extern void Parse_Viewpoint(Viewpoint *Given_Vp);
extern void Parse_Declare(void);
extern void Init_Viewpoint(Viewpoint *vp);
extern CONSTANT Find_Constant(void);
extern char *Get_Token_String(TOKEN Token_Id);
extern void Parse_Error(TOKEN Token_Id);
extern void Type_Error(void);
extern void Undeclared(void);

#endif
