#ifndef __HFIELD_H__
#define __HFIELD_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Boxes.h"

#define GIF 0 /* These are the types of image maps which can be used as a */
#define POT 1 /* height field. */
#define TGA 2

#include "geom/HeightField.h"
#include "geom/HeightFieldBlock.h"

extern Methods Height_Field_Methods;
extern HeightField *getHeightFieldShape(void);
extern void findHfMinMax(
    HeightField *H_Field, RGBAImage *Image, int Image_Type);
extern int allHeightfldIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int intersectHeightfld(Ray *Ray, HeightField *H_Field, DBL *Depth);
extern int insideHeightfld(Vector3D *Test_Point, SimpleBody *Object);
extern void heightFldNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyHeightfld(SimpleBody *Object);
extern void translateHeightfld(SimpleBody *Object, Vector3D *Vector);
extern void rotateHeightfld(SimpleBody *Object, Vector3D *Vector);
extern void scaleHeightfld(SimpleBody *Object, Vector3D *Vector);
extern void invertHeightfld(SimpleBody *Object);

#endif
