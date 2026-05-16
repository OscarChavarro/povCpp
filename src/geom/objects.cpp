/****************************************************************************
*                     objects.c
*
*  This module implements the methods for objects and composite objects.
*
*  from Persistence of Vision Raytracer 
*  Copyright 1992 Persistence of Vision Team
*---------------------------------------------------------------------------
*  Copying, distribution and legal info is in the file povlegal.doc which
*  should be distributed with this file. If povlegal.doc is not available
*  or for more info please contact:
*
*         Drew Wells [POV-Team Leader] 
*         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
*         Phone: (213) 254-4041
* 
* This program is based on the popular DKB raytracer version 2.12.
* DKBTrace was originally written by David K. Buck.
* DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
*
*****************************************************************************/

#include "geom/prioq.h"
#include "geom/objects.h"

/*===========================================================================*/

extern Ray *VP_Ray;
extern long Bounding_Region_Tests, Bounding_Region_Tests_Succeeded;
extern long Clipping_Region_Tests, Clipping_Region_Tests_Succeeded;
extern unsigned int Options;

Methods Composite_Methods =
{ Object_Intersect, All_Composite_Intersections,
  Inside_Composite_Object, NULL,
  Copy_Composite_Object,
  Translate_Composite_Object, Rotate_Composite_Object,
  Scale_Composite_Object, Invert_Composite_Object};

Methods Basic_Object_Methods =
{ Object_Intersect, All_Object_Intersections,
  Inside_Basic_Object, NULL,
  Copy_Basic_Object,
  Translate_Basic_Object, Rotate_Basic_Object,
  Scale_Basic_Object, Invert_Basic_Object};

/*===========================================================================*/

Intersection *
Object_Intersect(SimpleBody *Object, Ray *ray)
{
    Intersection *Local_Intersection, *Queue_Element;
    PriorityQueueNode *Depth_Queue;

    Depth_Queue = pq_pop(128);

    if ( (All_Intersections (Object, ray, Depth_Queue)) && 
         ((Queue_Element = Depth_Queue->getHighest()) != NULL) ) {
        if ( (Local_Intersection = new Intersection) == NULL ) {
            printf("Cannot allocate memory for local intersection\n");
            exit(1);
        }
        Local_Intersection->Point = Queue_Element->Point;
        Local_Intersection->Shape = Queue_Element->Shape;
        Local_Intersection->Depth = Queue_Element->Depth;
        Local_Intersection->Object = Queue_Element->Object;
        pq_push(Depth_Queue);
        return Local_Intersection;
    }
    else {
        pq_push(Depth_Queue);
        return NULL;
    }
}

int
All_Composite_Intersections(SimpleBody *Object, Ray *ray, PriorityQueueNode *Depth_Queue)
{
    register int Intersection_Found, Any_Intersection_Found;
    Geometry *Bounding_Shape;
    Geometry *Clipping_Shape;
    Intersection *Local_Intersection;
    SimpleBody *Local_Object;
    PriorityQueueNode *Local_Depth_Queue;

    for (Bounding_Shape = ((Composite *) Object) -> Bounding_Shapes ;
                  Bounding_Shape != NULL ;
                  Bounding_Shape = Bounding_Shape -> Next_Object) {

        Bounding_Region_Tests++;
        COOPERATE
        if ((Local_Intersection = Intersection ((SimpleBody *) Bounding_Shape, ray)) != NULL)
            delete Local_Intersection;
        else
            if (!Inside (&ray -> Initial, (SimpleBody *) Bounding_Shape))
                return (FALSE);
        Bounding_Region_Tests_Succeeded++;
    }

    Local_Depth_Queue = pq_pop(128);
    Any_Intersection_Found = FALSE;

    for (Local_Object = ((Composite *) Object) -> Objects ;
                  Local_Object != NULL ;
                  Local_Object = Local_Object -> Next_Object)

        All_Intersections (Local_Object, ray, Local_Depth_Queue);

    for (Local_Intersection = Local_Depth_Queue->getHighest();
          Local_Intersection != NULL ;
          Local_Depth_Queue->deleteHighest(),
          Local_Intersection = Local_Depth_Queue->getHighest()) {

        Intersection_Found = TRUE;

        for (Clipping_Shape = Object -> Clipping_Shapes ;
                                Clipping_Shape != NULL ;
                                Clipping_Shape = Clipping_Shape -> Next_Object) {
            Clipping_Region_Tests++;
            if (!Inside (&Local_Intersection->Point, (SimpleBody *) Clipping_Shape)) {
                Intersection_Found = FALSE;
                break;
            }
            Clipping_Region_Tests_Succeeded++;
        }

        if (Intersection_Found) {
            Depth_Queue->add(Local_Intersection);
            Any_Intersection_Found = TRUE;
        }
    }
    pq_push(Local_Depth_Queue);
    return (Any_Intersection_Found);
}

int
All_Object_Intersections(SimpleBody *Object, Ray *ray, PriorityQueueNode *Depth_Queue)
{
    int Intersection_Found, Any_Intersection_Found;
    Intersection *Local_Intersection;
    Geometry *Bounding_Shape;
    Geometry *Clipping_Shape;
    PriorityQueueNode *Local_Depth_Queue;

    for (Bounding_Shape = Object -> Bounding_Shapes ;
                  Bounding_Shape != NULL ;
                  Bounding_Shape = Bounding_Shape -> Next_Object) {

        Bounding_Region_Tests++;
        COOPERATE
        if ((Local_Intersection = Intersection ((SimpleBody *) Bounding_Shape, ray)) != NULL)
            delete Local_Intersection;
        else
            if (!Inside (&ray -> Initial, (SimpleBody *) Bounding_Shape))
                return (FALSE);
        Bounding_Region_Tests_Succeeded++;
    }

    Local_Depth_Queue = pq_pop(128);
    Any_Intersection_Found = FALSE;
    All_Intersections ((SimpleBody *)Object->Shape, ray, Local_Depth_Queue);

    for (Local_Intersection = Local_Depth_Queue->getHighest();
                     Local_Intersection != NULL ;
             Local_Depth_Queue->deleteHighest(),
             Local_Intersection = Local_Depth_Queue->getHighest()) {

        Intersection_Found = TRUE;

        for (Clipping_Shape = Object -> Clipping_Shapes ;
                         Clipping_Shape != NULL ;
                         Clipping_Shape = Clipping_Shape -> Next_Object) {

            Clipping_Region_Tests++;
            if (Options & DEBUGGING) {
                printf("Test (%.4f, %.4f, %.4f)\n",
                    Local_Intersection->Point.x,
                    Local_Intersection->Point.y,
                    Local_Intersection->Point.z);
            }
            if (!Inside (&Local_Intersection->Point, (SimpleBody *) Clipping_Shape)) {
                if (Options & DEBUGGING)
                    printf("not ok\n");
                Intersection_Found = FALSE;
                break;
            }
            Clipping_Region_Tests_Succeeded++; 
        }

        if (Intersection_Found) {
            if (Options & DEBUGGING)
                printf("ok\n");
            Depth_Queue->add(Local_Intersection);
            Any_Intersection_Found = TRUE;

        }
    }
    pq_push(Local_Depth_Queue);
    return (Any_Intersection_Found);
}

int
Inside_Basic_Object(Vector3D *Test_Point, SimpleBody *Object)
{
    Geometry *Bounding_Shape;
    Geometry *Clipping_Shape;

    for (Bounding_Shape = Object -> Bounding_Shapes ;
                  Bounding_Shape != NULL ;
                  Bounding_Shape = Bounding_Shape -> Next_Object)

        if (!Inside (Test_Point, (SimpleBody *) Bounding_Shape))
            return (FALSE);

    for (Clipping_Shape = Object -> Clipping_Shapes ;
                    Clipping_Shape != NULL ;
                    Clipping_Shape = Clipping_Shape -> Next_Object)

        if (!Inside (Test_Point, (SimpleBody *) Clipping_Shape))
            return (FALSE);

    if (Inside (Test_Point, (SimpleBody *) Object -> Shape))
        return (TRUE);
    return (FALSE);
}

int
Inside_Composite_Object(Vector3D *Test_Point, SimpleBody *Object)
{
    Geometry *Bounding_Shape;
    Geometry *Clipping_Shape;
    SimpleBody *Local_Object;

    for (Bounding_Shape = ((Composite *) Object) -> Bounding_Shapes ;
                  Bounding_Shape != NULL ;
                  Bounding_Shape = Bounding_Shape -> Next_Object)

        if (!Inside (Test_Point, (SimpleBody *) Bounding_Shape))
            return (FALSE);

    for (Clipping_Shape = ((Composite *) Object) -> Clipping_Shapes ;
                  Clipping_Shape != NULL ;
                  Clipping_Shape = Clipping_Shape -> Next_Object)

        if (!Inside (Test_Point, (SimpleBody *) Clipping_Shape))
            return (FALSE);

    for (Local_Object = ((Composite *) Object) -> Objects ;
                  Local_Object != NULL ;
                  Local_Object = Local_Object -> Next_Object)

        if (Inside (Test_Point, Local_Object))
            return (TRUE);

    return (FALSE);
}

void *
Copy_Basic_Object(SimpleBody *Object)
{
    Geometry *Local_Shape, *Copied_Shape;
    SimpleBody *New_Object;

    New_Object = Get_Object();
    *New_Object = *Object;
    New_Object -> Next_Object = NULL;
    New_Object -> Bounding_Shapes = NULL;
    New_Object -> Clipping_Shapes = NULL;
    for (Local_Shape = Object -> Bounding_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object) {

        Copied_Shape = (Geometry *) Copy((SimpleBody *) Local_Shape);
        Link ((SimpleBody *) Copied_Shape,
            (SimpleBody **) &(Copied_Shape -> Next_Object),
            (SimpleBody **) &(New_Object -> Bounding_Shapes));

        if ((Copied_Shape->Type == CSG_UNION_TYPE)
            || (Copied_Shape->Type == CSG_INTERSECTION_TYPE)
            || (Copied_Shape->Type == CSG_DIFFERENCE_TYPE))
            Set_CSG_Parents ((CSG *) Copied_Shape, New_Object);
    }

    for (Local_Shape = Object -> Clipping_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object) {

        Copied_Shape = (Geometry *) Copy((SimpleBody *) Local_Shape);
        Link ((SimpleBody *) Copied_Shape,
            (SimpleBody **) &(Copied_Shape -> Next_Object),
            (SimpleBody **) &(New_Object -> Clipping_Shapes));

        if ((Copied_Shape->Type == CSG_UNION_TYPE)
            || (Copied_Shape->Type == CSG_INTERSECTION_TYPE)
            || (Copied_Shape->Type == CSG_DIFFERENCE_TYPE))
            Set_CSG_Parents ((CSG *) Copied_Shape, New_Object);
    }

    New_Object -> Shape = (Geometry *) Copy((SimpleBody *) Object -> Shape);
    if ((New_Object->Shape->Type == CSG_UNION_TYPE)
        || (New_Object->Shape->Type == CSG_INTERSECTION_TYPE)
        || (New_Object->Shape->Type == CSG_DIFFERENCE_TYPE))
        Set_CSG_Parents ((CSG *) New_Object->Shape, New_Object);
    else
        New_Object->Shape->Parent_Object = New_Object;


    if (New_Object->Object_Texture != NULL)
        New_Object->Object_Texture = Copy_Texture (New_Object->Object_Texture);

    return ((void *)New_Object);
}

void *
Copy_Composite_Object(SimpleBody *Object)
{
    Composite *New_Object;
    Geometry *Local_Shape;
    SimpleBody *Local_Object, *Copied_Object;

    New_Object = Get_Composite_Object();
    *New_Object = *((Composite *) Object);
    New_Object -> Next_Object = NULL;
    New_Object -> Objects = NULL;
    for (Local_Object = ((Composite *) Object) -> Objects;
                  Local_Object != NULL ;
                  Local_Object = Local_Object -> Next_Object) {

        Copied_Object = (SimpleBody *) Copy(Local_Object);
        Link (Copied_Object,
            &(Copied_Object -> Next_Object),
            &(New_Object -> Objects));
    }

    New_Object -> Bounding_Shapes = NULL;
    for (Local_Shape = ((Composite *) Object) -> Bounding_Shapes;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object) {

        Copied_Object = (SimpleBody *) Copy((SimpleBody *) Local_Shape);
        Link (Copied_Object,
            &(Copied_Object -> Next_Object),
            (SimpleBody **) &(New_Object -> Bounding_Shapes));
    }
    New_Object -> Clipping_Shapes = NULL;
    for (Local_Shape = ((Composite *) Object) -> Clipping_Shapes;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object) {

        Copied_Object = (SimpleBody *) Copy((SimpleBody *) Local_Shape);
        Link (Copied_Object,
            &(Copied_Object -> Next_Object),
            (SimpleBody **) &(New_Object -> Clipping_Shapes));
    }
    return ((void *)New_Object);
}

void
Translate_Basic_Object(SimpleBody *Object, Vector3D *Vector)
{
    Geometry *Local_Shape;

    for (Local_Shape = Object -> Bounding_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Translate ((SimpleBody *) Local_Shape, Vector);

    for (Local_Shape = Object -> Clipping_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Translate ((SimpleBody *) Local_Shape, Vector);

    Translate ((SimpleBody *) Object -> Shape, Vector);

    Translate_Texture (&Object->Object_Texture, Vector);
}

void
Rotate_Basic_Object(SimpleBody *Object, Vector3D *Vector)
{
    Geometry *Local_Shape;
    Transformation transformation;

    for (Local_Shape = Object -> Bounding_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Rotate ((SimpleBody *) Local_Shape, Vector);

    for (Local_Shape = Object -> Clipping_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Rotate ((SimpleBody *) Local_Shape, Vector);

    Rotate ((SimpleBody *) Object -> Shape, Vector);
    Get_Rotation_Transformation (&transformation, Vector);

    Rotate_Texture (&Object->Object_Texture, Vector);
}

void
Scale_Basic_Object(SimpleBody *Object, Vector3D *Vector)
{
    Geometry *Local_Shape;

    for (Local_Shape = Object -> Bounding_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Scale ((SimpleBody *) Local_Shape, Vector);

    for (Local_Shape = Object -> Clipping_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Scale ((SimpleBody *) Local_Shape, Vector);

    Scale ((SimpleBody *) Object -> Shape, Vector);

    Scale_Texture (&Object->Object_Texture, Vector);
}

void
Translate_Composite_Object(SimpleBody *Object, Vector3D *Vector)
{
    SimpleBody *Local_Object;
    Geometry *Local_Shape;

    for (Local_Object = ((Composite *) Object) -> Objects;
                  Local_Object != NULL ;
                  Local_Object = Local_Object -> Next_Object)

        Translate (Local_Object, Vector);    

    for (Local_Shape = ((Composite *) Object) -> Bounding_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Translate ((SimpleBody *) Local_Shape, Vector);

    for (Local_Shape = ((Composite *) Object) -> Clipping_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Translate ((SimpleBody *) Local_Shape, Vector);
}

void
Rotate_Composite_Object(SimpleBody *Object, Vector3D *Vector)
{
    SimpleBody *Local_Object;
    Geometry *Local_Shape;

    for (Local_Object = ((Composite *) Object) -> Objects;
                  Local_Object != NULL ;
                  Local_Object = Local_Object -> Next_Object)

        Rotate (Local_Object, Vector);    

    for (Local_Shape = ((Composite *) Object) -> Bounding_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Rotate ((SimpleBody *) Local_Shape, Vector);

    for (Local_Shape = ((Composite *) Object) -> Clipping_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Rotate ((SimpleBody *) Local_Shape, Vector);
}

void
Scale_Composite_Object(SimpleBody *Object, Vector3D *Vector)
{
    SimpleBody *Local_Object;
    Geometry *Local_Shape;

    for (Local_Object = ((Composite *) Object) -> Objects;
                  Local_Object != NULL ;
                  Local_Object = Local_Object -> Next_Object)

        Scale (Local_Object, Vector);    

    for (Local_Shape = ((Composite *) Object) -> Bounding_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Scale ((SimpleBody *) Local_Shape, Vector);

    for (Local_Shape = ((Composite *) Object) -> Clipping_Shapes ;
                  Local_Shape != NULL ;
                  Local_Shape = Local_Shape -> Next_Object)

        Scale ((SimpleBody *) Local_Shape, Vector);
}

void
Invert_Basic_Object(SimpleBody *Object)
{
    Geometry *Local_Shape;

    for (Local_Shape = Object -> Bounding_Shapes ;
                  Local_Shape != NULL ;
          Local_Shape = Local_Shape -> Next_Object) {
        Invert ((SimpleBody *) Local_Shape);
    }

    for (Local_Shape = Object -> Clipping_Shapes ;
                  Local_Shape != NULL ;
          Local_Shape = Local_Shape -> Next_Object) {
        Invert ((SimpleBody *) Local_Shape);
    }
    Invert ((SimpleBody *) Object -> Shape);
}

void
Invert_Composite_Object(SimpleBody *Object)
{
    SimpleBody *Local_Object;
    Geometry *Local_Shape;

    for (Local_Object = ((Composite *)Object) -> Objects;
                  Local_Object != NULL ;
          Local_Object = Local_Object -> Next_Object) {
        Invert (Local_Object);    
    }

    for (Local_Shape = ((Composite *) Object) -> Bounding_Shapes ;
                  Local_Shape != NULL ;
          Local_Shape = Local_Shape -> Next_Object) {
        Invert ((SimpleBody *) Local_Shape);
    }

    for (Local_Shape = ((Composite *) Object) -> Clipping_Shapes ;
                  Local_Shape != NULL ;
          Local_Shape = Local_Shape -> Next_Object) {
        Invert ((SimpleBody *) Local_Shape);
    }
}

void Link(SimpleBody *New_Object, SimpleBody **Field, SimpleBody **Old_Object_List)
{
    *Field = *Old_Object_List;
    *Old_Object_List = New_Object;
}

SimpleBody *Get_Object()
{
    SimpleBody *New_Object;

    if ((New_Object = new SimpleBody()) == NULL) {
        Error ("Out of memory. Cannot allocate object");
    }

    New_Object -> Next_Object = NULL;
    /*  New_Object -> Next_Light_Source = NULL;*/
    New_Object -> Shape = NULL;
    New_Object -> Bounding_Shapes = NULL;
    New_Object -> Clipping_Shapes = NULL;
    New_Object -> Object_Texture = Default_Texture;

    New_Object->Object_Colour = NULL;

    New_Object -> No_Shadow_Flag = FALSE;
    New_Object -> Type = OBJECT_TYPE;
    New_Object -> methods = &Basic_Object_Methods;
    return (New_Object);
}
