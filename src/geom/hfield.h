#ifndef __HFIELD_H__
#define __HFIELD_H__

#include "common/frame.h"
#include "common/vector.h"
#include "common/povproto.h"
#include "geom/boxes.h"

#define GIF 0    /* These are the types of image maps which can be used as a */
#define POT 1    /* height field. */
#define TGA 2

class HeightFieldBlock
{
  public:
    DBL min_y, max_y;
};

class HeightField : public Geometry
{
  public:
    Transformation *transformation;
    Box *bounding_box;
    DBL Block_Size;
    DBL Inv_Blk_Size;
    HeightFieldBlock **Block;
    float **Map;
};

extern Methods Height_Field_Methods;
extern HeightField *Get_Height_Field_Shape(void);
extern void Find_Hf_Min_Max(HeightField *H_Field, RGBAImage *Image, int Image_Type);
extern int All_HeightFld_Intersections(SimpleBody *Object,Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Intersect_HeightFld(Ray *Ray,HeightField *H_Field,DBL *Depth);
extern int Inside_HeightFld(Vector3D *Test_Point,SimpleBody *Object);
extern void HeightFld_Normal(Vector3D *Result,SimpleBody *Object,Vector3D *Intersection_Point);
extern void *Copy_HeightFld(SimpleBody *Object);
extern void Translate_HeightFld(SimpleBody *Object,Vector3D *Vector);
extern void Rotate_HeightFld(SimpleBody *Object,Vector3D *Vector);
extern void Scale_HeightFld(SimpleBody *Object,Vector3D *Vector);
extern void Invert_HeightFld(SimpleBody *Object);

#endif
