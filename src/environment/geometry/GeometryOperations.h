#ifndef __GEOMETRY_OPERATIONS_H__
#define __GEOMETRY_OPERATIONS_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "java/util/PriorityQueue.h"
#include "common/dataStructures/PriorityQueuePool.h"
#include "environment/TransformableElement.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/SimpleBody.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/geometry/elements/GeometryTypes.h"

class GeometryOperations {
  public:
    static inline int
    allIntersections(TransformableElement *x, RayWithSegments *y, java::PriorityQueue<Intersection> *z)
    {
        return x->allIntersections(y, z);
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
    intersect(TransformableElement *x, RayWithSegments *y)
    {
        java::PriorityQueue<Intersection> * const depthQueue = PriorityQueuePool<Intersection>::pqPop(128);

        if (allIntersections(x, y, depthQueue) && depthQueue->size() > 0) {
            const Intersection queueElement = depthQueue->peek();
            Intersection * const localIntersection = new Intersection;
            if (localIntersection == nullptr) {
                Logger::reportMessage("GeometryOperations", Logger::FATAL_ERROR, "", "Cannot allocate memory for local intersection\n");
            }
            localIntersection->point = queueElement.point;
            localIntersection->Shape = queueElement.Shape;
            localIntersection->depth = queueElement.depth;
            localIntersection->Object = queueElement.Object;
            PriorityQueuePool<Intersection>::pqPush(depthQueue);
            return localIntersection;
        }
        PriorityQueuePool<Intersection>::pqPush(depthQueue);
        return nullptr;
    }

    static inline int
    inside(Vector3Dd *x, TransformableElement *y)
    {
        return y->inside(x);
    }

    static inline void
    normal(Vector3Dd *x, TransformableElement *y, Vector3Dd *z)
    {
        y->normal(x, z);
    }

    static inline void *
    copy(TransformableElement *x)
    {
        return x->copy();
    }

    static inline void
    translate(TransformableElement *x, Vector3Dd *y)
    {
        x->translate(y);
    }

    static inline void
    scale(TransformableElement *x, Vector3Dd *y)
    {
        x->scale(y);
    }

    static inline void
    rotate(TransformableElement *x, Vector3Dd *y)
    {
        x->rotate(y);
    }

    static inline void
    invert(TransformableElement *x)
    {
        x->invert();
    }
};

#endif
