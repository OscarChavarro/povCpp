/****************************************************************************
 *                     objects.c
 *
 *  This module implements the methods for objects and composite objects.
 *
 *****************************************************************************/

#include "environment/geometry/volume/compound/Composite.h"
#include "io/Parse.h"
#include "common/dataStructures/PriorityQueue.h"
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

    if ((GeometryOperations::allIntersections(object, ray, depthQueue)) &&
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
        if ((localIntersection = GeometryOperations::intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!GeometryOperations::inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool::pqPop(128);
    anyIntersectionFound = FALSE;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOperations::allIntersections(localObject, ray, localDepthQueue);
    }

    for (localIntersection = localDepthQueue->getHighest();
         localIntersection != nullptr; localDepthQueue->deleteHighest(),
        localIntersection = localDepthQueue->getHighest()) {

        intersectionFound = TRUE;

        for (clippingShape = object->Clipping_Shapes; clippingShape != nullptr;
             clippingShape = clippingShape->Next_Object) {
            clippingRegionTests++;
            if (!GeometryOperations::inside(
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
        if ((localIntersection = GeometryOperations::intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete localIntersection;
        } else if (!GeometryOperations::inside(&ray->Initial, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
        boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool::pqPop(128);
    anyIntersectionFound = FALSE;
    GeometryOperations::allIntersections((SimpleBody *)object->Shape, ray, localDepthQueue);

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
            if (!GeometryOperations::inside(
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
Composite::insideBasicObject(Vector3Dd *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;

    for (boundingShape = object->Bounding_Shapes; boundingShape != nullptr;
         boundingShape = boundingShape->Next_Object) {

        if (!GeometryOperations::inside(testPoint, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
    }

    for (clippingShape = object->Clipping_Shapes; clippingShape != nullptr;
         clippingShape = clippingShape->Next_Object) {

        if (!GeometryOperations::inside(testPoint, (SimpleBody *)clippingShape)) {
            return (FALSE);
        }
    }

    if (GeometryOperations::inside(testPoint, (SimpleBody *)object->Shape)) {
        return (TRUE);
    }
    return (FALSE);
}

int
Composite::insideCompositeObject(Vector3Dd *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;
    SimpleBody *localObject;

    for (boundingShape = ((Composite *)object)->Bounding_Shapes;
         boundingShape != nullptr; boundingShape = boundingShape->Next_Object) {

        if (!GeometryOperations::inside(testPoint, (SimpleBody *)boundingShape)) {
            return (FALSE);
        }
    }

    for (clippingShape = ((Composite *)object)->Clipping_Shapes;
         clippingShape != nullptr; clippingShape = clippingShape->Next_Object) {

        if (!GeometryOperations::inside(testPoint, (SimpleBody *)clippingShape)) {
            return (FALSE);
        }
    }

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        if (GeometryOperations::inside(testPoint, localObject)) {
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

        copiedShape = (Geometry *)GeometryOperations::copy((SimpleBody *)localShape);
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

        copiedShape = (Geometry *)GeometryOperations::copy((SimpleBody *)localShape);
        ObjectUtils::link((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->Next_Object),
            (SimpleBody **)&(newObject->Clipping_Shapes));

        if ((copiedShape->Type == CSG_UNION_TYPE) ||
            (copiedShape->Type == CSG_INTERSECTION_TYPE) ||
            (copiedShape->Type == CSG_DIFFERENCE_TYPE)) {
            CSG::setCsgParents((CSG *)copiedShape, newObject);
        }
    }

    newObject->Shape = (Geometry *)GeometryOperations::copy((SimpleBody *)object->Shape);
    if ((newObject->Shape->Type == CSG_UNION_TYPE) ||
        (newObject->Shape->Type == CSG_INTERSECTION_TYPE) ||
        (newObject->Shape->Type == CSG_DIFFERENCE_TYPE)) {
        CSG::setCsgParents((CSG *)newObject->Shape, newObject);
    } else {
        newObject->Shape->Parent_Object = newObject;
    }

    if (newObject->Object_Texture != nullptr) {
        newObject->Object_Texture = TextureParser::copyTexture(newObject->Object_Texture);
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

    newObject = SceneFactory::getCompositeObject();
    *newObject = *((Composite *)object);
    newObject->Next_Object = nullptr;
    newObject->Objects = nullptr;
    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        copiedObject = (SimpleBody *)GeometryOperations::copy(localObject);
        ObjectUtils::link(copiedObject, &(copiedObject->Next_Object), &(newObject->Objects));
    }

    newObject->Bounding_Shapes = nullptr;
    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        copiedObject = (SimpleBody *)GeometryOperations::copy((SimpleBody *)localShape);
        ObjectUtils::link(copiedObject, &(copiedObject->Next_Object),
            (SimpleBody **)&(newObject->Bounding_Shapes));
    }
    newObject->Clipping_Shapes = nullptr;
    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        copiedObject = (SimpleBody *)GeometryOperations::copy((SimpleBody *)localShape);
        ObjectUtils::link(copiedObject, &(copiedObject->Next_Object),
            (SimpleBody **)&(newObject->Clipping_Shapes));
    }
    return ((void *)newObject);
}

void
Composite::translateBasicObject(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }

    GeometryOperations::translate((SimpleBody *)object->Shape, vector);

    TextureUtils::translateTexture(&object->Object_Texture, vector);
}

void
Composite::rotateBasicObject(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;
    Transformation transformation;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }

    GeometryOperations::rotate((SimpleBody *)object->Shape, vector);
    Transformation::getRotationTransformation(&transformation, vector);

    TextureUtils::rotateTexture(&object->Object_Texture, vector);
}

void
Composite::scaleBasicObject(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }

    GeometryOperations::scale((SimpleBody *)object->Shape, vector);

    TextureUtils::scaleTexture(&object->Object_Texture, vector);
}

void
Composite::translateCompositeObject(SimpleBody *object, Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOperations::translate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }
}

void
Composite::rotateCompositeObject(SimpleBody *object, Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOperations::rotate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }
}

void
Composite::scaleCompositeObject(SimpleBody *object, Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {

        GeometryOperations::scale(localObject, vector);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }
}

void
Composite::invertBasicObject(SimpleBody *object)
{
    Geometry *localShape;

    for (localShape = object->Bounding_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        GeometryOperations::invert((SimpleBody *)localShape);
    }

    for (localShape = object->Clipping_Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        GeometryOperations::invert((SimpleBody *)localShape);
    }
    GeometryOperations::invert((SimpleBody *)object->Shape);
}

void
Composite::invertCompositeObject(SimpleBody *object)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->Objects; localObject != nullptr;
         localObject = localObject->Next_Object) {
        GeometryOperations::invert(localObject);
    }

    for (localShape = ((Composite *)object)->Bounding_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {
        GeometryOperations::invert((SimpleBody *)localShape);
    }

    for (localShape = ((Composite *)object)->Clipping_Shapes;
         localShape != nullptr; localShape = localShape->Next_Object) {
        GeometryOperations::invert((SimpleBody *)localShape);
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
        ParseErrorReporter::Error("Out of memory. Cannot allocate object");
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
