/****************************************************************************
 *                         csg.c
 *
 *  This module implements routines for constructive solid geometry.
 *
 *****************************************************************************/

#include "environment/geometry/volume/compound/CSG.h"
#include "io/Parse.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "io/Parse.h"
Methods CSG_Union_Methods = {Composite::objectIntersect, CSG::allCsgUnionIntersections,
    CSG::insideCsgUnion, nullptr, CSG::copyCsg, CSG::translateCsg, CSG::rotateCsg, CSG::scaleCsg,
    CSG::invertCsg};

Methods CSG_Intersection_Methods = {Composite::objectIntersect,
    CSG::allCsgIntersectIntersections, CSG::insideCsgIntersection, nullptr, CSG::copyCsg,
    CSG::translateCsg, CSG::rotateCsg, CSG::scaleCsg, CSG::invertCsg};

extern Ray *vpRay;
int
CSG::allCsgUnionIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    register int intersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    intersectionFound = FALSE;
    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {
        if (GeometryOperations::allIntersections((SimpleBody *)localShape, ray, depthQueue)) {
            intersectionFound = TRUE;
        }
    }

    return (intersectionFound);
}

int
CSG::allCsgIntersectIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    int intersectionFound;
    int anyIntersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;
    Geometry *shape2;
    PriorityQueueNode *localDepthQueue;
    Intersection *localIntersection;

    localDepthQueue = PriorityQueuePool::pqPop(128);

    anyIntersectionFound = FALSE;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::allIntersections((SimpleBody *)localShape, ray, localDepthQueue);

        for (localIntersection = localDepthQueue->getHighest();
             localIntersection != nullptr; localDepthQueue->deleteHighest(),
            localIntersection = localDepthQueue->getHighest()) {

            intersectionFound = TRUE;

            for (shape2 = shape->Shapes; shape2 != nullptr;
                 shape2 = shape2->Next_Object) {

                if (shape2 != localShape) {
                    if (!GeometryOperations::inside(
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
CSG::insideCsgUnion(Vector3Dd *testPoint, SimpleBody *object)
{
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        if (GeometryOperations::inside(testPoint, (SimpleBody *)localShape)) {
            return (TRUE);
        }
    }
    return (FALSE);
}

int
CSG::insideCsgIntersection(Vector3Dd *testPoint, SimpleBody *object)
{
    Geometry *localShape;
    CSG *shape = (CSG *)object;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        if (!GeometryOperations::inside(testPoint, (SimpleBody *)localShape)) {
            return (FALSE);
        }
    }

    return (TRUE);
}

void *
CSG::copyCsg(SimpleBody *object)
{
    CSG *shape = (CSG *)object;
    CSG *newShape;
    Geometry *localShape;
    Geometry *copiedShape;

    newShape = SceneFactory::getCsgShape();
    newShape->methods = shape->methods;
    newShape->Type = shape->Type;
    newShape->Next_Object = nullptr;
    newShape->Shapes = nullptr;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        copiedShape = (Geometry *)GeometryOperations::copy((SimpleBody *)localShape);
        ObjectUtils::link((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->Next_Object),
            (SimpleBody **)&(newShape->Shapes));
    }
    return ((void *)newShape);
}

void
CSG::translateCsg(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }
}

void
CSG::rotateCsg(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }
}

void
CSG::scaleCsg(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }
}

void
CSG::invertCsg(SimpleBody *object)
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

        GeometryOperations::invert((SimpleBody *)localShape);
    }
}

void
CSG::setCsgParents(CSG *shape, SimpleBody *object)
{
    Geometry *localShape;

    for (localShape = shape->Shapes; localShape != nullptr;
         localShape = localShape->Next_Object) {

        localShape->Parent_Object = object;
        if ((localShape->Type == CSG_UNION_TYPE) ||
            (localShape->Type == CSG_INTERSECTION_TYPE)) {
            CSG::setCsgParents((CSG *)localShape, object);
        }
    }
}
