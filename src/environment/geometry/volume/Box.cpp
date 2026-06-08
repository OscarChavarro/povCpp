/****************************************************************************
 *                     boxes.c
 *
 *  This module implements the box primitive.
 *
 *  This file was written by Alexander Enzmann.  He wrote the code for
 *  boxes and generously provided us these enhancements.
 *
 *****************************************************************************/

#include "environment/geometry/volume/Box.h"
#include "common/logger/Logger.h"
#include "common/Config.h"
#include "common/Statistics.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include <cstring>
Methods Box::methodTable = {Box::allBoxIntersections,
    Box::insideBox, Box::boxNormal, Box::copyBox, Box::translateBox,
    Box::rotateBox, Box::scaleBox, Box::invertBox};


int
Box::closeTo(double x, double y)
{
    return fabs(x - y) < Config::kEpsilon ? 1 : 0;
}
int
Box::allBoxIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    Intersection localElement;
    bool intersectionFound;
    Box *shape = (Box *)object;

    intersectionFound = false;
    if (Box::intersectBoxx(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = nullptr;
        intersectionPoint = ray->direction.multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->position);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = nullptr;
            intersectionPoint = ray->direction.multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->position);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = true;
        }
    }
    return (intersectionFound);
}

int
Box::intersectBoxx(
    RayWithSegments *ray, Box *box, double *depth1, double *depth2)
{
    double t;
    double tmin;
    double tmax;
    Vector3Dd p;
    Vector3Dd d;

    Statistics::global().rayBoxTests++;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MInverseTransformVector(
            &p, &ray->position, box->Transform);
        Transformation::MInvTransVector(&d, &ray->direction, box->Transform);
    } else {
        p = Vector3Dd(ray->position.x(), ray->position.y(), ray->position.z());
        d = Vector3Dd(ray->direction.x(), ray->direction.y(), ray->direction.z());
    }

    tmin = 0.0;
    tmax = HUGE_VAL;

    /* Sides first */
    if (d.x() < -Config::kEpsilon) {
        t = (box->bounds[0].x() - p.x()) / d.x();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].x() - p.x()) / d.x();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.x() > Config::kEpsilon) {
        t = (box->bounds[1].x() - p.x()) / d.x();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].x() - p.x()) / d.x();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.x() < box->bounds[0].x() || p.x() > box->bounds[1].x()) {
        return 0;
    }

    /* Check Top/Bottom */
    if (d.y() < -Config::kEpsilon) {
        t = (box->bounds[0].y() - p.y()) / d.y();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].y() - p.y()) / d.y();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.y() > Config::kEpsilon) {
        t = (box->bounds[1].y() - p.y()) / d.y();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].y() - p.y()) / d.y();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.y() < box->bounds[0].y() || p.y() > box->bounds[1].y()) {
        return 0;
    }

    /* Now front/back */
    if (d.z() < -Config::kEpsilon) {
        t = (box->bounds[0].z() - p.z()) / d.z();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].z() - p.z()) / d.z();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.z() > Config::kEpsilon) {
        t = (box->bounds[1].z() - p.z()) / d.z();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].z() - p.z()) / d.z();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.z() < box->bounds[0].z() || p.z() > box->bounds[1].z()) {
        return 0;
    }

    *depth1 = tmin;
    *depth2 = tmax;

    /* Logger::info("Box intersects: %g, %g\n", *Depth1, *Depth2); */
    if ((*depth1 < GeometryConstants::Small_Tolerance) || (*depth1 > GeometryConstants::Max_Distance)) {
        if ((*depth2 < GeometryConstants::Small_Tolerance) || (*depth2 > GeometryConstants::Max_Distance)) {
            return (false);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < GeometryConstants::Small_Tolerance) || (*depth2 > GeometryConstants::Max_Distance)) {
        *depth2 = *depth1;
    }

    Statistics::global().rayBoxTestsSucceeded++;
    return (true);
}

int
Box::insideBox(Vector3Dd *testPoint, SimpleBody *object)
{
    Vector3Dd newPoint;
    Box *box = (Box *)object;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MInverseTransformVector(
            &newPoint, testPoint, box->Transform);
    } else {
        newPoint = *testPoint;
    }

    /* Test to see if we are inside the box */
    if (newPoint.x() < box->bounds[0].x() || newPoint.x() > box->bounds[1].x()) {
        return ((int)box->Inverted);
    }
    if (newPoint.y() < box->bounds[0].y() || newPoint.y() > box->bounds[1].y()) {
        return ((int)box->Inverted);
    }
    if (newPoint.z() < box->bounds[0].z() || newPoint.z() > box->bounds[1].z()) {
        return ((int)box->Inverted);
    }
    /* Inside the box */
    return 1 - box->Inverted;
}

void
Box::boxNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    Vector3Dd newPoint;
    Box *box = (Box *)object;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MInverseTransformVector(
            &newPoint, intersectionPoint, box->Transform);
    } else {
        newPoint = Vector3Dd(
            intersectionPoint->x(), intersectionPoint->y(), intersectionPoint->z());
    }

    *result = Vector3Dd(0.0, 0.0, 0.0);
    if (Box::closeTo(newPoint.x(), box->bounds[1].x())) {
        *result = result->withX(1.0);
    } else if (Box::closeTo(newPoint.x(), box->bounds[0].x())) {
        *result = result->withX(-1.0);
    } else if (Box::closeTo(newPoint.y(), box->bounds[1].y())) {
        *result = result->withY(1.0);
    } else if (Box::closeTo(newPoint.y(), box->bounds[0].y())) {
        *result = result->withY(-1.0);
    } else if (Box::closeTo(newPoint.z(), box->bounds[1].z())) {
        *result = result->withZ(1.0);
    } else if (closeTo(newPoint.z(), box->bounds[0].z())) {
        *result = result->withZ(-1.0);
    } else {
        /* Bad result, should we do something with it? */
        *result = result->withX(1.0);
    }

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MTransNormal(result, result, box->Transform);
        *result = (*result).normalizedFast();
    }
}

void *
Box::copyBox(SimpleBody *object)
{
    Box *newShape;
    Transformation *tr;

    newShape = new Box;
    *newShape = *((Box *)object);
    newShape->nextObject = nullptr;

    /* Copy any associated transformation */
    if (newShape->Transform != nullptr) {
        tr = Transformation::getTransformation();
        *tr = *(newShape->Transform);
        newShape->Transform = tr;
    }

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureUtils::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Box::translateBox(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transform;
    Box *box = (Box *)object;
    if (box->Transform == nullptr) {
        box->Transform = Transformation::getTransformation();
    }
    Transformation::getTranslationTransformation(&transform, vector);
    Transformation::composeTransformations(box->Transform, &transform);

    TextureUtils::translateTexture(&((Box *)object)->Shape_Texture, vector);
}

void
Box::rotateBox(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transform;
    Box *box = (Box *)object;
    if (box->Transform == nullptr) {
        box->Transform = Transformation::getTransformation();
    }
    Transformation::getRotationTransformation(&transform, vector);
    Transformation::composeTransformations(box->Transform, &transform);

    TextureUtils::rotateTexture(&((Box *)object)->Shape_Texture, vector);
}

void
Box::scaleBox(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transform;
    Box *box = (Box *)object;
    if (box->Transform == nullptr) {
        box->Transform = Transformation::getTransformation();
    }
    Transformation::getScalingTransformation(&transform, vector);
    Transformation::composeTransformations(box->Transform, &transform);

    TextureUtils::scaleTexture(&((Box *)object)->Shape_Texture, vector);
}

void
Box::invertBox(SimpleBody *object)
{
    ((Box *)object)->Inverted = !((Box *)object)->Inverted;
}
