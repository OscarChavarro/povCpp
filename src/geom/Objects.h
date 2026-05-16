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
extern Intersection *Object_Intersect(SimpleBody *Object, Ray *Ray);
extern int All_Composite_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int All_Object_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Inside_Basic_Object(Vector3D *point, SimpleBody *Object);
extern int Inside_Composite_Object(Vector3D *point, SimpleBody *Object);
extern void *Copy_Basic_Object(SimpleBody *Object);
extern void *Copy_Composite_Object(SimpleBody *Object);
extern void Translate_Basic_Object(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Basic_Object(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Basic_Object(SimpleBody *Object, Vector3D *Vector);
extern void Translate_Composite_Object(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Composite_Object(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Composite_Object(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Basic_Object(SimpleBody *Object);
extern void Invert_Composite_Object(SimpleBody *Object);

extern void Set_CSG_Parents(CSG *Shape, SimpleBody *Object);

extern void Link(
    SimpleBody *New_Object, SimpleBody **Field, SimpleBody **Old_Object_List);

extern SimpleBody *Get_Object(void);
extern Composite *Get_Composite_Object(void);

#endif
