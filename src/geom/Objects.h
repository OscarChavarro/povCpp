#ifndef __OBJECTS_H__
#define __OBJECTS_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Csg.h"
#include "geom/Geometry.h"

class Composite {
  public:
    Methods *methods;
    int Type;
    SimpleBody *Next_Object;
    /*    SimpleBody *Next_Light_Source;*/
    Geometry *Bounding_Shapes;
    Geometry *Clipping_Shapes;
    SimpleBody *Objects;
};

extern Methods Composite_Methods;
extern Methods Basic_Object_Methods;
extern Intersection *objectIntersect(SimpleBody *Object, Ray *Ray);
extern int allCompositeIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int allObjectIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int insideBasicObject(Vector3D *point, SimpleBody *Object);
extern int insideCompositeObject(Vector3D *point, SimpleBody *Object);
extern void *copyBasicObject(SimpleBody *Object);
extern void *copyCompositeObject(SimpleBody *Object);
extern void translateBasicObject(SimpleBody *Object, Vector3D *Vector);
extern void rotateBasicObject(SimpleBody *Object, Vector3D *Vector);
extern void scaleBasicObject(SimpleBody *Object, Vector3D *Vector);
extern void translateCompositeObject(SimpleBody *Object, Vector3D *Vector);
extern void rotateCompositeObject(SimpleBody *Object, Vector3D *Vector);
extern void scaleCompositeObject(SimpleBody *Object, Vector3D *Vector);
extern void invertBasicObject(SimpleBody *Object);
extern void invertCompositeObject(SimpleBody *Object);

extern void setCsgParents(CSG *Shape, SimpleBody *Object);

extern void Link(
    SimpleBody *New_Object, SimpleBody **Field, SimpleBody **Old_Object_List);

extern SimpleBody *getObject(void);
extern Composite *getCompositeObject(void);

#endif
