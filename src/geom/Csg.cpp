/****************************************************************************
 *                         csg.c
 *
 *  This module implements routines for constructive solid geometry.
 *
 *****************************************************************************/

#include "geom/Csg.h"
#include "geom/Objects.h"
Methods CSG_Union_Methods = {objectIntersect, allCsgUnionIntersections,
    insideCsgUnion, nullptr, copyCsg, translateCsg, rotateCsg, scaleCsg,
    invertCsg};

Methods CSG_Intersection_Methods = {objectIntersect,
    allCsgIntersectIntersections, insideCsgIntersection, nullptr, copyCsg,
    translateCsg, rotateCsg, scaleCsg, invertCsg};

extern Ray *vpRay;
int
allCsgUnionIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    register int intersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    intersectionFound = FALSE;
    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        if (allIntersections((SimpleBody *)localShape, ray, depthQueue)) {
            intersectionFound = TRUE;
        }
    }

    return (intersectionFound);
}

int
allCsgIntersectIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    int intersectionFound;
    int anyIntersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;
    Geometry *shape2;
    PriorityQueueNode *localDepthQueue;
    Intersection *localIntersection;

    localDepthQueue = pqPop(128);

    anyIntersectionFound = FALSE;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        allIntersections((SimpleBody *)localShape, ray, localDepthQueue);

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
insideCsgUnion(Vector3D *testPoint, SimpleBody *object)
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
insideCsgIntersection(Vector3D *testPoint, SimpleBody *object)
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
copyCsg(SimpleBody *object)
{
    CSG *shape = (CSG *)object;
    CSG *newShape;
    Geometry *localShape;
    Geometry *copiedShape;

    newShape = getCsgShape();
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
translateCsg(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Translate((SimpleBody *)localShape, vector);
    }
}

void
rotateCsg(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Rotate((SimpleBody *)localShape, vector);
    }
}

void
scaleCsg(SimpleBody *object, Vector3D *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        Scale((SimpleBody *)localShape, vector);
    }
}

void
invertCsg(SimpleBody *object)
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
setCsgParents(CSG *shape, SimpleBody *object)
{
    Geometry *localShape;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        localShape->Parent_Object = object;
        if ((localShape->Type == CSG_UNION_TYPE) ||
            (localShape->Type == CSG_INTERSECTION_TYPE)) {
            setCsgParents((CSG *)localShape, object);
        }
    }
}
