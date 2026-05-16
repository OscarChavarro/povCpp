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
    Inside_CSG_Union, nullptr, Copy_CSG, Translate_CSG, Rotate_CSG, Scale_CSG,
    Invert_CSG};

Methods CSG_Intersection_Methods = {Object_Intersect,
    All_CSG_Intersect_Intersections, Inside_CSG_Intersection, nullptr, Copy_CSG,
    Translate_CSG, Rotate_CSG, Scale_CSG, Invert_CSG};

extern Ray *vpRay;

/*===========================================================================*/

int
All_CSG_Union_Intersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    register int intersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    intersectionFound = FALSE;
    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        if (All_Intersections((SimpleBody *)localShape, ray, depthQueue)) {
            intersectionFound = TRUE;
        }
    }

    return (intersectionFound);
}

int
All_CSG_Intersect_Intersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    int intersectionFound;
    int anyIntersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;
    Geometry *shape2;
    PriorityQueueNode *localDepthQueue;
    Intersection *localIntersection;

    localDepthQueue = pq_pop(128);

    anyIntersectionFound = FALSE;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        All_Intersections((SimpleBody *)localShape, ray, localDepthQueue);

        for (localIntersection = localDepthQueue->getHighest();
             localIntersection != nullptr; localDepthQueue->deleteHighest(),
            localIntersection = localDepthQueue->getHighest()) {

            intersectionFound = TRUE;

            for (shape2 = shape->Shapes; shape2 != nullptr;
                 shape2 = shape2->Next_Object) {

                if (shape2 != localShape) {
                    if (!Inside(
                            &localIntersection->Point, (SimpleBody *)shape2)) {
                        intersectionFound = FALSE;
                        break;
                    }
                }
            }

            if (intersectionFound) {
                depthQueue->add(localIntersection);
                anyIntersectionFound = TRUE;
            }
        }
    }

    localDepthQueue->pushBackToPool();

    return (anyIntersectionFound);
}

int
Inside_CSG_Union(Vector3D *testPoint, SimpleBody *object)
{
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        if (Inside(testPoint, (SimpleBody *)localShape)) {
            return (TRUE);
        }
    }
    return (FALSE);
}

int
Inside_CSG_Intersection(Vector3D *testPoint, SimpleBody *object)
{
    Geometry *localShape;
    CSG *shape = (CSG *)object;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        if (!Inside(testPoint, (SimpleBody *)localShape)) {
            return (FALSE);
        }
    }

    return (TRUE);
}

void *
Copy_CSG(SimpleBody *object)
{
    CSG *shape = (CSG *)object;
    CSG *newShape;
    Geometry *localShape;
    Geometry *copiedShape;

    newShape = Get_CSG_Shape();
    newShape->methods = shape->methods;
    newShape->Type = shape->Type;
    newShape->Next_Object = nullptr;
    newShape->Shapes = nullptr;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        copiedShape = (Geometry *)Copy((SimpleBody *)localShape);
        Link((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->Next_Object),
            (SimpleBody **)&(newShape->Shapes));
    }
    return ((void *)newShape);
}

void
Translate_CSG(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Translate((SimpleBody *)localShape, vector);
    }
}

void
Rotate_CSG(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Rotate((SimpleBody *)localShape, vector);
    }
}

void
Scale_CSG(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Scale((SimpleBody *)localShape, vector);
    }
}

void
Invert_CSG(SimpleBody *object)
{
    Geometry *localShape;
    CSG *csg = (CSG *)object;

    if (csg->Type == CSG_INTERSECTION_TYPE) {
        csg->Type = CSG_UNION_TYPE;
        csg->methods = &CSG_Union_Methods;
    } else if (csg->Type == CSG_UNION_TYPE) {
        csg->Type = CSG_INTERSECTION_TYPE;
        csg->methods = &CSG_Intersection_Methods;
    }

    for (localShape = csg->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Invert((SimpleBody *)localShape);
    }
}

void
Set_CSG_Parents(CSG *shape, SimpleBody *object)
{
    Geometry *localShape;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        localShape->Parent_Object = object;
        if ((localShape->Type == CSG_UNION_TYPE) ||
            (localShape->Type == CSG_INTERSECTION_TYPE)) {
            Set_CSG_Parents((CSG *)localShape, object);
        }
    }
}
