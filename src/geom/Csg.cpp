/****************************************************************************
 *                         csg.c
 *
 *  This module implements routines for constructive solid geometry.
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

#include "geom/Csg.h"
#include "geom/Objects.h"

/*===========================================================================*/

Methods CSG_Union_Methods = {Object_Intersect, All_CSG_Union_Intersections,
    Inside_CSG_Union, NULL, Copy_CSG, Translate_CSG, Rotate_CSG, Scale_CSG,
    Invert_CSG};

Methods CSG_Intersection_Methods = {Object_Intersect,
    All_CSG_Intersect_Intersections, Inside_CSG_Intersection, NULL, Copy_CSG,
    Translate_CSG, Rotate_CSG, Scale_CSG, Invert_CSG};

extern Ray *VP_Ray;

/*===========================================================================*/

int
All_CSG_Union_Intersections(
    SimpleBody *Object, Ray *ray, PriorityQueueNode *Depth_Queue)
{
    register int Intersection_Found;
    CSG *Shape = (CSG *)Object;
    Geometry *Local_Shape;

    Intersection_Found = FALSE;
    for (Local_Shape = Shape->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {
        if (All_Intersections((SimpleBody *)Local_Shape, ray, Depth_Queue)) {
            Intersection_Found = TRUE;
        }
    }

    return (Intersection_Found);
}

int
All_CSG_Intersect_Intersections(
    SimpleBody *Object, Ray *ray, PriorityQueueNode *Depth_Queue)
{
    int Intersection_Found, Any_Intersection_Found;
    CSG *Shape = (CSG *)Object;
    Geometry *Local_Shape, *Shape2;
    PriorityQueueNode *Local_Depth_Queue;
    Intersection *Local_Intersection;

    Local_Depth_Queue = pq_pop(128);

    Any_Intersection_Found = FALSE;

    for (Local_Shape = Shape->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        All_Intersections((SimpleBody *)Local_Shape, ray, Local_Depth_Queue);

        for (Local_Intersection = Local_Depth_Queue->getHighest();
             Local_Intersection != NULL; Local_Depth_Queue->deleteHighest(),
            Local_Intersection = Local_Depth_Queue->getHighest()) {

            Intersection_Found = TRUE;

            for (Shape2 = Shape->Shapes; Shape2 != NULL;
                 Shape2 = Shape2->Next_Object) {

                if (Shape2 != Local_Shape) {
                    if (!Inside(
                            &Local_Intersection->Point, (SimpleBody *)Shape2)) {
                        Intersection_Found = FALSE;
                        break;
                    }
                }
            }

            if (Intersection_Found) {
                Depth_Queue->add(Local_Intersection);
                Any_Intersection_Found = TRUE;
            }
        }
    }

    Local_Depth_Queue->pushBackToPool();

    return (Any_Intersection_Found);
}

int
Inside_CSG_Union(Vector3D *Test_Point, SimpleBody *Object)
{
    CSG *Shape = (CSG *)Object;
    Geometry *Local_Shape;

    for (Local_Shape = Shape->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        if (Inside(Test_Point, (SimpleBody *)Local_Shape)) {
            return (TRUE);
        }
    }
    return (FALSE);
}

int
Inside_CSG_Intersection(Vector3D *Test_Point, SimpleBody *Object)
{
    Geometry *Local_Shape;
    CSG *Shape = (CSG *)Object;

    for (Local_Shape = Shape->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        if (!Inside(Test_Point, (SimpleBody *)Local_Shape)) {
            return (FALSE);
        }
    }

    return (TRUE);
}

void *
Copy_CSG(SimpleBody *Object)
{
    CSG *Shape = (CSG *)Object;
    CSG *New_Shape;
    Geometry *Local_Shape, *Copied_Shape;

    New_Shape = Get_CSG_Shape();
    New_Shape->methods = Shape->methods;
    New_Shape->Type = Shape->Type;
    New_Shape->Next_Object = NULL;
    New_Shape->Shapes = NULL;

    for (Local_Shape = Shape->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        Copied_Shape = (Geometry *)Copy((SimpleBody *)Local_Shape);
        Link((SimpleBody *)Copied_Shape,
            (SimpleBody **)&(Copied_Shape->Next_Object),
            (SimpleBody **)&(New_Shape->Shapes));
    }
    return ((void *)New_Shape);
}

void
Translate_CSG(SimpleBody *Object, Vector3D *Vector)
{
    Geometry *Local_Shape;

    for (Local_Shape = ((CSG *)Object)->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        Translate((SimpleBody *)Local_Shape, Vector);
    }
}

void
Rotate_CSG(SimpleBody *Object, Vector3D *Vector)
{
    Geometry *Local_Shape;

    for (Local_Shape = ((CSG *)Object)->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        Rotate((SimpleBody *)Local_Shape, Vector);
    }
}

void
Scale_CSG(SimpleBody *Object, Vector3D *Vector)
{
    Geometry *Local_Shape;

    for (Local_Shape = ((CSG *)Object)->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        Scale((SimpleBody *)Local_Shape, Vector);
    }
}

void
Invert_CSG(SimpleBody *Object)
{
    Geometry *Local_Shape;
    CSG *Csg = (CSG *)Object;

    if (Csg->Type == CSG_INTERSECTION_TYPE) {
        Csg->Type = CSG_UNION_TYPE;
        Csg->methods = &CSG_Union_Methods;
    } else if (Csg->Type == CSG_UNION_TYPE) {
        Csg->Type = CSG_INTERSECTION_TYPE;
        Csg->methods = &CSG_Intersection_Methods;
    }

    for (Local_Shape = Csg->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        Invert((SimpleBody *)Local_Shape);
    }
}

void
Set_CSG_Parents(CSG *Shape, SimpleBody *Object)
{
    Geometry *Local_Shape;

    for (Local_Shape = Shape->Shapes; Local_Shape != NULL;
         Local_Shape = Local_Shape->Next_Object) {

        Local_Shape->Parent_Object = Object;
        if ((Local_Shape->Type == CSG_UNION_TYPE) ||
            (Local_Shape->Type == CSG_INTERSECTION_TYPE)) {
            Set_CSG_Parents((CSG *)Local_Shape, Object);
        }
    }
}
