/****************************************************************************
 *                     boxes.c
 *
 *  This module implements the box primitive.
 *
 *  This file was written by Alexander Enzmann.  He wrote the code for
 *  boxes and generously provided us these enhancements.
 *
 *****************************************************************************/

#include "geom/Boxes.h"
#include "io/Parse.h"
#include "geom/Objects.h"
Methods Box_Methods = {Composite::objectIntersect, Box::allBoxIntersections, Box::insideBox,
    Box::boxNormal, Box::copyBox, Box::translateBox, Box::rotateBox, Box::scaleBox, Box::invertBox};


extern Ray *vpRay;
extern long rayBoxTests, rayBoxTestsSucceeded;

int
Box::closeTo(double x, double y)
{
    return fabs(x - y) < kEpsilon ? 1 : 0;
}
int
Box::allBoxIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    double depth1, depth2;
    Vector3D intersectionPoint;
    Intersection localElement;
    register int intersectionFound;
    Box *shape = (Box *)object;

    intersectionFound = FALSE;
    if (Box::intersectBoxx(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = shape->Parent_Object;
        VectorOps::vScale(intersectionPoint, ray->Direction, depth1);
        VectorOps::vAdd(intersectionPoint, intersectionPoint, ray->Initial);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = TRUE;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = shape->Parent_Object;
            VectorOps::vScale(intersectionPoint, ray->Direction, depth2);
            VectorOps::vAdd(intersectionPoint, intersectionPoint, ray->Initial);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = TRUE;
        }
    }
    return (intersectionFound);
}

int
Box::intersectBoxx(Ray *ray, Box *box, double *depth1, double *depth2)
{
    double t, tmin, tmax;
    Vector3D p;
    Vector3D d;

    rayBoxTests++;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MInverseTransformVector(&p, &ray->Initial, box->Transform);
        Transformation::MInvTransVector(&d, &ray->Direction, box->Transform);
    } else {
        p.x = ray->Initial.x;
        p.y = ray->Initial.y;
        p.z = ray->Initial.z;
        d.x = ray->Direction.x;
        d.y = ray->Direction.y;
        d.z = ray->Direction.z;
    }

    tmin = 0.0;
    tmax = HUGE_VAL;

    /* Sides first */
    if (d.x < -kEpsilon) {
        t = (box->bounds[0].x - p.x) / d.x;
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].x - p.x) / d.x;
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.x > kEpsilon) {
        t = (box->bounds[1].x - p.x) / d.x;
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].x - p.x) / d.x;
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.x < box->bounds[0].x || p.x > box->bounds[1].x) {
        return 0;
    }

    /* Check Top/Bottom */
    if (d.y < -kEpsilon) {
        t = (box->bounds[0].y - p.y) / d.y;
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].y - p.y) / d.y;
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.y > kEpsilon) {
        t = (box->bounds[1].y - p.y) / d.y;
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].y - p.y) / d.y;
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.y < box->bounds[0].y || p.y > box->bounds[1].y) {
        return 0;
    }

    /* Now front/back */
    if (d.z < -kEpsilon) {
        t = (box->bounds[0].z - p.z) / d.z;
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].z - p.z) / d.z;
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.z > kEpsilon) {
        t = (box->bounds[1].z - p.z) / d.z;
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].z - p.z) / d.z;
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.z < box->bounds[0].z || p.z > box->bounds[1].z) {
        return 0;
    }

    *depth1 = tmin;
    *depth2 = tmax;

    /* printf("Box intersects: %g, %g\n", *Depth1, *Depth2); */
    if ((*depth1 < Small_Tolerance) || (*depth1 > Max_Distance)) {
        if ((*depth2 < Small_Tolerance) || (*depth2 > Max_Distance)) {
            return (FALSE);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < Small_Tolerance) || (*depth2 > Max_Distance)) {
        *depth2 = *depth1;
    }

    rayBoxTestsSucceeded++;
    return (TRUE);
}

int
Box::insideBox(Vector3D *testPoint, SimpleBody *object)
{
    Vector3D newPoint;
    Box *box = (Box *)object;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MInverseTransformVector(&newPoint, testPoint, box->Transform);
    } else {
        newPoint = *testPoint;
    }

    /* Test to see if we are inside the box */
    if (newPoint.x < box->bounds[0].x || newPoint.x > box->bounds[1].x) {
        return ((int)box->Inverted);
    }
    if (newPoint.y < box->bounds[0].y || newPoint.y > box->bounds[1].y) {
        return ((int)box->Inverted);
    }
    if (newPoint.z < box->bounds[0].z || newPoint.z > box->bounds[1].z) {
        return ((int)box->Inverted);
    }
    /* Inside the box */
    return 1 - box->Inverted;
}

void
Box::boxNormal(Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    Vector3D newPoint;
    Box *box = (Box *)object;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MInverseTransformVector(&newPoint, intersectionPoint, box->Transform);
    } else {
        newPoint.x = intersectionPoint->x;
        newPoint.y = intersectionPoint->y;
        newPoint.z = intersectionPoint->z;
    }

    result->x = 0.0;
    result->y = 0.0;
    result->z = 0.0;
    if (Box::closeTo(newPoint.x, box->bounds[1].x)) {
        result->x = 1.0;
    } else if (Box::closeTo(newPoint.x, box->bounds[0].x)) {
        result->x = -1.0;
    } else if (Box::closeTo(newPoint.y, box->bounds[1].y)) {
        result->y = 1.0;
    } else if (Box::closeTo(newPoint.y, box->bounds[0].y)) {
        result->y = -1.0;
    } else if (Box::closeTo(newPoint.z, box->bounds[1].z)) {
        result->z = 1.0;
    } else if (closeTo(newPoint.z, box->bounds[0].z)) {
        result->z = -1.0;
    } else {
        /* Bad result, should we do something with it? */
        result->x = 1.0;
    }

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        Transformation::MTransNormal(result, result, box->Transform);
        VectorOps::vNormalize(*result, *result);
    }
}

void *
Box::copyBox(SimpleBody *object)
{
    Box *newShape;
    Transformation *tr;

    newShape = SceneFactory::getBoxShape();
    *newShape = *((Box *)object);
    newShape->Next_Object = nullptr;

    /* Copy any associated transformation */
    if (newShape->Transform != nullptr) {
        tr = Transformation::getTransformation();
        memcpy(tr, newShape->Transform, sizeof(Transformation));
        newShape->Transform = tr;
    }

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = TextureParser::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Box::translateBox(SimpleBody *object, Vector3D *vector)
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
Box::rotateBox(SimpleBody *object, Vector3D *vector)
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
Box::scaleBox(SimpleBody *object, Vector3D *vector)
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
    ((Box *)object)->Inverted = 1 - ((Box *)object)->Inverted;
}
