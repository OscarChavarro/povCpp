#ifndef __GEOMETRY_OPERATIONS_H__
#define __GEOMETRY_OPERATIONS_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "common/dataStructures/PriorityQueue.h"
#include "environment/material/Material.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/Methods.h"
#include "environment/geometry/SimpleBody.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/geometry/elements/GeometryTypes.h"

class GeometryOperations {
  public:
    static inline int
    allIntersections(SimpleBody *x, RayWithSegments *y, PriorityQueueNode *z)
    {
        return ((*((x)->methods->allIntersectionsMethod))(x, y, z));
    }

    /**
    Generic "nearest hit" routine. Formerly the per-object
    intersectionMethod slot, which was identical for every shape
    (Composite::objectIntersect / its Triangle clone). The slot was
    removed; this is now the single dispatch point. It works purely
    through the object's own allIntersectionsMethod, so no shape needs
    to depend on Composite just to fill a method-table entry.
    */
    static inline Intersection *
    intersect(SimpleBody *x, RayWithSegments *y)
    {
        PriorityQueueNode * const depthQueue = IntersectionPriorityQueuePool::pqPop(128);

        if (allIntersections(x, y, depthQueue) && depthQueue->size() > 0) {
            const Intersection queueElement = depthQueue->peek();
            Intersection * const localIntersection = new Intersection;
            if (localIntersection == nullptr) {
                Logger::reportMessage("GeometryOperations", Logger::FATAL_ERROR, "", "Cannot allocate memory for local intersection\n");
            }
            localIntersection->Point = queueElement.Point;
            localIntersection->Shape = queueElement.Shape;
            localIntersection->Depth = queueElement.Depth;
            localIntersection->Object = queueElement.Object;
            IntersectionPriorityQueuePool::pqPush(depthQueue);
            return localIntersection;
        }
        IntersectionPriorityQueuePool::pqPush(depthQueue);
        return nullptr;
    }

    static inline int
    inside(Vector3Dd *x, SimpleBody *y)
    {
        return ((*((y)->methods->insideMethod))(x, y));
    }

    static inline void
    normal(Vector3Dd *x, SimpleBody *y, Vector3Dd *z)
    {
        ((*((y)->methods->normalMethod))(x, y, z));
    }

    static inline void *
    copy(SimpleBody *x)
    {
        return ((*((x)->methods->copyMethod))(x));
    }

    static inline void
    translate(SimpleBody *x, Vector3Dd *y)
    {
        ((*((x)->methods->translateMethod))(x, y));
    }

    static inline void
    scale(SimpleBody *x, Vector3Dd *y)
    {
        ((*((x)->methods->scaleMethod))(x, y));
    }

    static inline void
    rotate(SimpleBody *x, Vector3Dd *y)
    {
        ((*((x)->methods->rotateMethod))(x, y));
    }

    static inline void
    invert(SimpleBody *x)
    {
        ((*((x)->methods->invertMethod))(x));
    }
};

#endif
