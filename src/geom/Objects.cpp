/****************************************************************************
 *                     objects.c
 *
 *  This module implements the methods for objects and composite objects.
 *
 *****************************************************************************/

#include "geom/Objects.h"
#include "io/Parse.h"
#include "geom/PrioQ.h"
extern Ray *vpRay;
extern long boundingRegionTests, boundingRegionTestsSucceeded;
extern long clippingRegionTests, clippingRegionTestsSucceeded;
extern unsigned int Options;

Methods Composite_Methods = {Composite::objectIntersect, Composite::allCompositeIntersections,
    Composite::insideCompositeObject, nullptr, Composite::copyCompositeObject,
    Composite::translateCompositeObject, Composite::rotateCompositeObject, Composite::scaleCompositeObject,
    Composite::invertCompositeObject};

Methods Basic_Object_Methods = {Composite::objectIntersect, Composite::allObjectIntersections,
    Composite::insideBasicObject, nullptr, Composite::copyBasicObject, Composite::translateBasicObject,
    Composite::rotateBasicObject, Composite::scaleBasicObject, Composite::invertBasicObject};

Intersection *
Composite::objectIntersect(SimpleBody *object, Ray *ray)
{
    Intersection *localIntersection;
    Intersection *queueElement;
    PriorityQueueNode *depthQueue;

    depthQueue = PriorityQueuePool::pqPop(128);

    if ((GeometryOps::allIntersections(object, ray, depthQueue)) &&
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
Composite::allCompositeIntersections(
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
        if ((localIntersection = GeometryOps::intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!GeometryOps::inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool::pqPop(128);
    anyIntersectionFound = FALSE;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOps::allIntersections(localObject, ray, localDepthQueue);
    }

    for (localIntersection = localDepthQueue->getHighest();
         localIntersection != nullptr; localDepthQueue->deleteHighest(),
        localIntersection = localDepthQueue->getHighest()) {

        intersectionFound = TRUE;

        for (clippingShape = object->Clipping_Shapes; clippingShape != nullptr;
             clippingShape = clippingShape->Next_Object) {
            clippingRegionTests++;
            if (!GeometryOps::inside(
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
Composite::allObjectIntersections(
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
        if ((localIntersection = GeometryOps::intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!GeometryOps::inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool::pqPop(128);
    anyIntersectionFound = FALSE;
    GeometryOps::allIntersections((SimpleBody *)object->Shape, ray, localDepthQueue);

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
            if (!GeometryOps::inside(
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
Composite::insideBasicObject(Vector3D *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;

    for (boundingShape = object->Bounding_Shapes; boundingShape != nullptr;
         boundingShape = boundingShape->Next_Object) {

        if (!GeometryOps::inside(testPoint, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
    }

    for (clippingShape = object->Clipping_Shapes; clippingShape != nullptr;
         clippingShape = clippingShape->Next_Object) {

        if (!GeometryOps::inside(testPoint, (SimpleBody *)clippingShape)) {
            return (FALSE);
        }
    }

    if (GeometryOps::inside(testPoint, (SimpleBody *)object->Shape)) {
        return (TRUE);
    }
    return (FALSE);
}

int
Composite::insideCompositeObject(Vector3D *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;
    SimpleBody *localObject;

    for (boundingShape = ((Composite *)object)->Bounding_Shapes;
         boundingShape != nullptr; boundingShape = boundingShape->Next_Object) {

        if (!GeometryOps::inside(testPoint, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
    }

    for (clippingShape = ((Composite *)object)->Clipping_Shapes;
         clippingShape != nullptr; clippingShape = clippingShape->Next_Object) {

        if (!GeometryOps::inside(testPoint, (SimpleBody *)clippingShape)) {
            return (FALSE);
        }
    }

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        if (GeometryOps::inside(testPoint, localObject)) {
            return (TRUE);
        }
    }

    return (FALSE);
}

void *
Composite::copyBasicObject(SimpleBody *object)
{
    Geometry *localShape;
    Geometry *copiedShape;
    SimpleBody *newObject;

    newObject = ObjectUtils::getObject();
    *newObject = *object;
    newObject->Next_Object = nullptr;
    newObject->Bounding_Shapes = nullptr;
    newObject->Clipping_Shapes = nullptr;
    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        copiedShape = (Geometry *)GeometryOps::copy((SimpleBody *)localShape);
        ObjectUtils::link((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->Next_Object),
            (SimpleBody **)&(newObject->Bounding_Shapes));

        if ((copiedShape->Type == CSG_UNION_TYPE) ||
            (copiedShape->Type == CSG_INTERSECTION_TYPE) ||
            (copiedShape->Type == CSG_DIFFERENCE_TYPE)) {
            CSG::setCsgParents((CSG *)copiedShape, newObject);
        }
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        copiedShape = (Geometry *)GeometryOps::copy((SimpleBody *)localShape);
        ObjectUtils::link((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->Next_Object),
            (SimpleBody **)&(newObject->Clipping_Shapes));

        if ((copiedShape->Type == CSG_UNION_TYPE) ||
            (copiedShape->Type == CSG_INTERSECTION_TYPE) ||
            (copiedShape->Type == CSG_DIFFERENCE_TYPE)) {
            CSG::setCsgParents((CSG *)copiedShape, newObject);
        }
    }

    newObject->Shape = (Geometry *)GeometryOps::copy((SimpleBody *)object->Shape);
    if ((newObject->Shape->Type == CSG_UNION_TYPE) ||
        (newObject->Shape->Type == CSG_INTERSECTION_TYPE) ||
        (newObject->Shape->Type == CSG_DIFFERENCE_TYPE)) {
        CSG::setCsgParents((CSG *)newObject->Shape, newObject);
    } else {
        newObject->Shape->Parent_Object = newObject;
    }

    if (newObject->Object_Texture != nullptr) {
        newObject->Object_Texture = copyTexture(newObject->Object_Texture);
    }

    return ((void *)newObject);
}

void *
Composite::copyCompositeObject(SimpleBody *object)
{
    Composite *newObject;
    Geometry *localShape;
    SimpleBody *localObject;
    SimpleBody *copiedObject;

    newObject = ParseFactory::getCompositeObject();
    *newObject = *((Composite *)object);
    newObject->Next_Object = nullptr;
    newObject->Objects = nullptr;
    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        copiedObject = (SimpleBody *)GeometryOps::copy(localObject);
        ObjectUtils::link(copiedObject, &(copiedObject->Next_Object), &(newObject->Objects));
    }

    newObject->Bounding_Shapes = nullptr;
    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        copiedObject = (SimpleBody *)GeometryOps::copy((SimpleBody *)localShape);
        ObjectUtils::link(copiedObject, &(copiedObject->Next_Object),
            (SimpleBody **)&(newObject->Bounding_Shapes));
    }
    newObject->Clipping_Shapes = nullptr;
    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        copiedObject = (SimpleBody *)GeometryOps::copy((SimpleBody *)localShape);
        ObjectUtils::link(copiedObject, &(copiedObject->Next_Object),
            (SimpleBody **)&(newObject->Clipping_Shapes));
    }
    return ((void *)newObject);
}

void
Composite::translateBasicObject(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOps::translate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOps::translate((SimpleBody *)localShape, vector);
    }

    GeometryOps::translate((SimpleBody *)object->Shape, vector);

    translateTexture(&object->Object_Texture, vector);
}

void
Composite::rotateBasicObject(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;
    Transformation transformation;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOps::rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOps::rotate((SimpleBody *)localShape, vector);
    }

    GeometryOps::rotate((SimpleBody *)object->Shape, vector);
    Transformation::getRotationTransformation(&transformation, vector);

    rotateTexture(&object->Object_Texture, vector);
}

void
Composite::scaleBasicObject(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOps::scale((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOps::scale((SimpleBody *)localShape, vector);
    }

    GeometryOps::scale((SimpleBody *)object->Shape, vector);

    scaleTexture(&object->Object_Texture, vector);
}

void
Composite::translateCompositeObject(SimpleBody *object, Vector3D *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOps::translate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOps::translate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOps::translate((SimpleBody *)localShape, vector);
    }
}

void
Composite::rotateCompositeObject(SimpleBody *object, Vector3D *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOps::rotate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOps::rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOps::rotate((SimpleBody *)localShape, vector);
    }
}

void
Composite::scaleCompositeObject(SimpleBody *object, Vector3D *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOps::scale(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOps::scale((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOps::scale((SimpleBody *)localShape, vector);
    }
}

void
Composite::invertBasicObject(SimpleBody *object)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        GeometryOps::invert((SimpleBody *)localShape);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        GeometryOps::invert((SimpleBody *)localShape);
    }
    GeometryOps::invert((SimpleBody *)object->Shape);
}

void
Composite::invertCompositeObject(SimpleBody *object)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {
        GeometryOps::invert(localObject);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {
        GeometryOps::invert((SimpleBody *)localShape);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {
        GeometryOps::invert((SimpleBody *)localShape);
    }
}

void
ObjectUtils::link(SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

SimpleBody *
ObjectUtils::getObject()
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
