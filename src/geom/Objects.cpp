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

#include "geom/Objects.h"
#include "geom/PrioQ.h"

/*===========================================================================*/

extern Ray *vpRay;
extern long boundingRegionTests, boundingRegionTestsSucceeded;
extern long clippingRegionTests, clippingRegionTestsSucceeded;
extern unsigned int Options;

Methods Composite_Methods = {Object_Intersect, All_Composite_Intersections,
    Inside_Composite_Object, nullptr, Copy_Composite_Object,
    Translate_Composite_Object, Rotate_Composite_Object, Scale_Composite_Object,
    Invert_Composite_Object};

Methods Basic_Object_Methods = {Object_Intersect, All_Object_Intersections,
    Inside_Basic_Object, nullptr, Copy_Basic_Object, Translate_Basic_Object,
    Rotate_Basic_Object, Scale_Basic_Object, Invert_Basic_Object};

/*===========================================================================*/

Intersection *
Object_Intersect(SimpleBody *object, Ray *ray)
{
    Intersection *localIntersection;
    Intersection *queueElement;
    PriorityQueueNode *depthQueue;

    depthQueue = pq_pop(128);

    if ((All_Intersections(object, ray, depthQueue)) &&
        ((queueElement = depthQueue->getHighest()) != nullptr)) {
        if ((localIntersection = new Intersection) == nullptr) {
            printf("Cannot allocate memory for local intersection\n");
            exit(1);
        }
        localIntersection->Point = queueElement->Point;
        localIntersection->Shape = queueElement->Shape;
        localIntersection->Depth = queueElement->Depth;
        localIntersection->Object = queueElement->Object;
        depthQueue->pushBackToPool();
        return localIntersection;
    }
    depthQueue->pushBackToPool();
    return nullptr;
}

int
All_Composite_Intersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    register int intersectionFound;
    register int anyIntersectionFound;
    Geometry *boundingShape;
    Geometry *clippingShape;
    Intersection *localIntersection;
    SimpleBody *localObject;
    PriorityQueueNode *localDepthQueue;

    for (boundingShape = ((Composite *)object)->Bounding_Shapes;
         boundingShape != nullptr; boundingShape = boundingShape->Next_Object) {

        boundingRegionTests++;
        COOPERATE
        if ((localIntersection = Intersection(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!Inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = pq_pop(128);
    anyIntersectionFound = FALSE;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        All_Intersections(localObject, ray, localDepthQueue);
    }

    for (localIntersection = localDepthQueue->getHighest();
         localIntersection != nullptr; localDepthQueue->deleteHighest(),
        localIntersection = localDepthQueue->getHighest()) {

        intersectionFound = TRUE;

        for (clippingShape = object->Clipping_Shapes; clippingShape != nullptr;
             clippingShape = clippingShape->Next_Object) {
            clippingRegionTests++;
            if (!Inside(
                    &localIntersection->Point, (SimpleBody *)clippingShape)) {
                intersectionFound = FALSE;
                break;
            }
            clippingRegionTestsSucceeded++;
        }

        if (intersectionFound) {
            depthQueue->add(localIntersection);
            anyIntersectionFound = TRUE;
        }
    }
    localDepthQueue->pushBackToPool();
    return (anyIntersectionFound);
}

int
All_Object_Intersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    int intersectionFound;
    int anyIntersectionFound;
    Intersection *localIntersection;
    Geometry *boundingShape;
    Geometry *clippingShape;
    PriorityQueueNode *localDepthQueue;

    for (boundingShape = object->Bounding_Shapes; boundingShape != nullptr;
         boundingShape = boundingShape->Next_Object) {

        boundingRegionTests++;
        COOPERATE
        if ((localIntersection = Intersection(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!Inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = pq_pop(128);
    anyIntersectionFound = FALSE;
    All_Intersections((SimpleBody *)object->Shape, ray, localDepthQueue);

    for (localIntersection = localDepthQueue->getHighest();
         localIntersection != nullptr; localDepthQueue->deleteHighest(),
        localIntersection = localDepthQueue->getHighest()) {

        intersectionFound = TRUE;

        for (clippingShape = object->Clipping_Shapes; clippingShape != nullptr;
             clippingShape = clippingShape->Next_Object) {

            clippingRegionTests++;
            if (Options & DEBUGGING) {
                printf("Test (%.4f, %.4f, %.4f)\n", localIntersection->Point.x,
                    localIntersection->Point.y, localIntersection->Point.z);
            }
            if (!Inside(
                    &localIntersection->Point, (SimpleBody *)clippingShape)) {
                if (Options & DEBUGGING) {
                    printf("not ok\n");
                }
                intersectionFound = FALSE;
                break;
            }
            clippingRegionTestsSucceeded++;
        }

        if (intersectionFound) {
            if (Options & DEBUGGING) {
                printf("ok\n");
            }
            depthQueue->add(localIntersection);
            anyIntersectionFound = TRUE;
        }
    }
    localDepthQueue->pushBackToPool();
    return (anyIntersectionFound);
}

int
Inside_Basic_Object(Vector3D *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;

    for (boundingShape = object->Bounding_Shapes; boundingShape != nullptr;
         boundingShape = boundingShape->Next_Object) {

        if (!Inside(testPoint, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
    }

    for (clippingShape = object->Clipping_Shapes; clippingShape != nullptr;
         clippingShape = clippingShape->Next_Object) {

        if (!Inside(testPoint, (SimpleBody *)clippingShape)) {
            return (FALSE);
        }
    }

    if (Inside(testPoint, (SimpleBody *)object->Shape)) {
        return (TRUE);
    }
    return (FALSE);
}

int
Inside_Composite_Object(Vector3D *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;
    SimpleBody *localObject;

    for (boundingShape = ((Composite *)object)->Bounding_Shapes;
         boundingShape != nullptr; boundingShape = boundingShape->Next_Object) {

        if (!Inside(testPoint, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
    }

    for (clippingShape = ((Composite *)object)->Clipping_Shapes;
         clippingShape != nullptr; clippingShape = clippingShape->Next_Object) {

        if (!Inside(testPoint, (SimpleBody *)clippingShape)) {
            return (FALSE);
        }
    }

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        if (Inside(testPoint, localObject)) {
            return (TRUE);
        }
    }

    return (FALSE);
}

void *
Copy_Basic_Object(SimpleBody *object)
{
    Geometry *localShape;
    Geometry *copiedShape;
    SimpleBody *newObject;

    newObject = Get_Object();
    *newObject = *object;
    newObject->Next_Object = nullptr;
    newObject->Bounding_Shapes = nullptr;
    newObject->Clipping_Shapes = nullptr;
    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        copiedShape = (Geometry *)Copy((SimpleBody *)localShape);
        Link((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->Next_Object),
            (SimpleBody **)&(newObject->Bounding_Shapes));

        if ((copiedShape->Type == CSG_UNION_TYPE) ||
            (copiedShape->Type == CSG_INTERSECTION_TYPE) ||
            (copiedShape->Type == CSG_DIFFERENCE_TYPE)) {
            Set_CSG_Parents((CSG *)copiedShape, newObject);
        }
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        copiedShape = (Geometry *)Copy((SimpleBody *)localShape);
        Link((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->Next_Object),
            (SimpleBody **)&(newObject->Clipping_Shapes));

        if ((copiedShape->Type == CSG_UNION_TYPE) ||
            (copiedShape->Type == CSG_INTERSECTION_TYPE) ||
            (copiedShape->Type == CSG_DIFFERENCE_TYPE)) {
            Set_CSG_Parents((CSG *)copiedShape, newObject);
        }
    }

    newObject->Shape = (Geometry *)Copy((SimpleBody *)object->Shape);
    if ((newObject->Shape->Type == CSG_UNION_TYPE) ||
        (newObject->Shape->Type == CSG_INTERSECTION_TYPE) ||
        (newObject->Shape->Type == CSG_DIFFERENCE_TYPE)) {
        Set_CSG_Parents((CSG *)newObject->Shape, newObject);
    } else {
        newObject->Shape->Parent_Object = newObject;
    }

    if (newObject->Object_Texture != nullptr) {
        newObject->Object_Texture = Copy_Texture(newObject->Object_Texture);
    }

    return ((void *)newObject);
}

void *
Copy_Composite_Object(SimpleBody *object)
{
    Composite *newObject;
    Geometry *localShape;
    SimpleBody *localObject;
    SimpleBody *copiedObject;

    newObject = Get_Composite_Object();
    *newObject = *((Composite *)object);
    newObject->Next_Object = nullptr;
    newObject->Objects = nullptr;
    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        copiedObject = (SimpleBody *)Copy(localObject);
        Link(copiedObject, &(copiedObject->Next_Object), &(newObject->Objects));
    }

    newObject->Bounding_Shapes = nullptr;
    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        copiedObject = (SimpleBody *)Copy((SimpleBody *)localShape);
        Link(copiedObject, &(copiedObject->Next_Object),
            (SimpleBody **)&(newObject->Bounding_Shapes));
    }
    newObject->Clipping_Shapes = nullptr;
    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        copiedObject = (SimpleBody *)Copy((SimpleBody *)localShape);
        Link(copiedObject, &(copiedObject->Next_Object),
            (SimpleBody **)&(newObject->Clipping_Shapes));
    }
    return ((void *)newObject);
}

void
Translate_Basic_Object(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Translate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Translate((SimpleBody *)localShape, vector);
    }

    Translate((SimpleBody *)object->Shape, vector);

    Translate_Texture(&object->Object_Texture, vector);
}

void
Rotate_Basic_Object(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;
    Transformation transformation;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Rotate((SimpleBody *)localShape, vector);
    }

    Rotate((SimpleBody *)object->Shape, vector);
    Get_Rotation_Transformation(&transformation, vector);

    Rotate_Texture(&object->Object_Texture, vector);
}

void
Scale_Basic_Object(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Scale((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Scale((SimpleBody *)localShape, vector);
    }

    Scale((SimpleBody *)object->Shape, vector);

    Scale_Texture(&object->Object_Texture, vector);
}

void
Translate_Composite_Object(SimpleBody *object, Vector3D *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        Translate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        Translate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        Translate((SimpleBody *)localShape, vector);
    }
}

void
Rotate_Composite_Object(SimpleBody *object, Vector3D *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        Rotate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        Rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        Rotate((SimpleBody *)localShape, vector);
    }
}

void
Scale_Composite_Object(SimpleBody *object, Vector3D *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        Scale(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        Scale((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        Scale((SimpleBody *)localShape, vector);
    }
}

void
Invert_Basic_Object(SimpleBody *object)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        Invert((SimpleBody *)localShape);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        Invert((SimpleBody *)localShape);
    }
    Invert((SimpleBody *)object->Shape);
}

void
Invert_Composite_Object(SimpleBody *object)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {
        Invert(localObject);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {
        Invert((SimpleBody *)localShape);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {
        Invert((SimpleBody *)localShape);
    }
}

void
Link(SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

SimpleBody *
Get_Object()
{
    SimpleBody *newObject;

    if ((newObject = new SimpleBody()) == nullptr) {
        Error("Out of memory. Cannot allocate object");
    }

    newObject->Next_Object = nullptr;
    /*  New_Object -> Next_Light_Source = NULL;*/
    newObject->Shape = nullptr;
    newObject->Bounding_Shapes = nullptr;
    newObject->Clipping_Shapes = nullptr;
    newObject->Object_Texture = Default_Texture;

    newObject->Object_Colour = nullptr;

    newObject->No_Shadow_Flag = FALSE;
    newObject->Type = OBJECT_TYPE;
    newObject->methods = &Basic_Object_Methods;
    return (newObject);
}
