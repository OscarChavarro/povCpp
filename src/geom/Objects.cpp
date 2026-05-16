/****************************************************************************
 *                     objects.c
 *
 *  This module implements the methods for objects and composite objects.
 *
 *****************************************************************************/

#include "geom/Objects.h"
#include "geom/PrioQ.h"
extern Ray *vpRay;
extern long boundingRegionTests, boundingRegionTestsSucceeded;
extern long clippingRegionTests, clippingRegionTestsSucceeded;
extern unsigned int Options;

Methods Composite_Methods = {objectIntersect, allCompositeIntersections,
    insideCompositeObject, nullptr, copyCompositeObject,
    translateCompositeObject, rotateCompositeObject, scaleCompositeObject,
    invertCompositeObject};

Methods Basic_Object_Methods = {objectIntersect, allObjectIntersections,
    insideBasicObject, nullptr, copyBasicObject, translateBasicObject,
    rotateBasicObject, scaleBasicObject, invertBasicObject};
Intersection *
objectIntersect(SimpleBody *object, Ray *ray)
{
    Intersection *localIntersection;
    Intersection *queueElement;
    PriorityQueueNode *depthQueue;

    depthQueue = pqPop(128);

    if ((allIntersections(object, ray, depthQueue)) &&
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
allCompositeIntersections(
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
        cooperate();
        if ((localIntersection = intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!Inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = pqPop(128);
    anyIntersectionFound = FALSE;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        allIntersections(localObject, ray, localDepthQueue);
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
allObjectIntersections(
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
        cooperate();
        if ((localIntersection = intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!Inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = pqPop(128);
    anyIntersectionFound = FALSE;
    allIntersections((SimpleBody *)object->Shape, ray, localDepthQueue);

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
insideBasicObject(Vector3D *testPoint, SimpleBody *object)
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
insideCompositeObject(Vector3D *testPoint, SimpleBody *object)
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
copyBasicObject(SimpleBody *object)
{
    Geometry *localShape;
    Geometry *copiedShape;
    SimpleBody *newObject;

    newObject = getObject();
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
            setCsgParents((CSG *)copiedShape, newObject);
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
            setCsgParents((CSG *)copiedShape, newObject);
        }
    }

    newObject->Shape = (Geometry *)Copy((SimpleBody *)object->Shape);
    if ((newObject->Shape->Type == CSG_UNION_TYPE) ||
        (newObject->Shape->Type == CSG_INTERSECTION_TYPE) ||
        (newObject->Shape->Type == CSG_DIFFERENCE_TYPE)) {
        setCsgParents((CSG *)newObject->Shape, newObject);
    } else {
        newObject->Shape->Parent_Object = newObject;
    }

    if (newObject->Object_Texture != nullptr) {
        newObject->Object_Texture = copyTexture(newObject->Object_Texture);
    }

    return ((void *)newObject);
}

void *
copyCompositeObject(SimpleBody *object)
{
    Composite *newObject;
    Geometry *localShape;
    SimpleBody *localObject;
    SimpleBody *copiedObject;

    newObject = getCompositeObject();
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
translateBasicObject(SimpleBody *object, Vector3D *vector)
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

    translateTexture(&object->Object_Texture, vector);
}

void
rotateBasicObject(SimpleBody *object, Vector3D *vector)
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
    getRotationTransformation(&transformation, vector);

    rotateTexture(&object->Object_Texture, vector);
}

void
scaleBasicObject(SimpleBody *object, Vector3D *vector)
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

    scaleTexture(&object->Object_Texture, vector);
}

void
translateCompositeObject(SimpleBody *object, Vector3D *vector)
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
rotateCompositeObject(SimpleBody *object, Vector3D *vector)
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
scaleCompositeObject(SimpleBody *object, Vector3D *vector)
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
invertBasicObject(SimpleBody *object)
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
invertCompositeObject(SimpleBody *object)
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
getObject()
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
