/****************************************************************************
 *                     boxes.c
 *
 *  This module implements the box primitive.
 *
 *  This file was written by Alexander Enzmann.  He wrote the code for
 *  boxes and generously provided us these enhancements.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1991 Persistence of Vision Team
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

#include "geom/Boxes.h"
#include "geom/Objects.h"

/*===========================================================================*/

Methods Box_Methods = {objectIntersect, allBoxIntersections, insideBox,
    boxNormal, copyBox, translateBox, rotateBox, scaleBox, invertBox};

extern Box *getBoxShape();

extern Ray *vpRay;
extern long rayBoxTests, rayBoxTestsSucceeded;

#define close(x, y) (fabs(x - y) < EPSILON ? 1 : 0)

/*===========================================================================*/

int
allBoxIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    DBL depth1, depth2;
    Vector3D intersectionPoint;
    Intersection localElement;
    register int intersectionFound;
    Box *shape = (Box *)object;

    intersectionFound = FALSE;
    if (intersectBoxx(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = shape->Parent_Object;
        VScale(intersectionPoint, ray->Direction, depth1);
        VAdd(intersectionPoint, intersectionPoint, ray->Initial);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = TRUE;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = shape->Parent_Object;
            VScale(intersectionPoint, ray->Direction, depth2);
            VAdd(intersectionPoint, intersectionPoint, ray->Initial);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = TRUE;
        }
    }
    return (intersectionFound);
}

int
intersectBoxx(Ray *ray, Box *box, DBL *depth1, DBL *depth2)
{
    DBL t, tmin, tmax;
    Vector3D p;
    Vector3D d;

    rayBoxTests++;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        MInverseTransformVector(&p, &ray->Initial, box->Transform);
        MInvTransVector(&d, &ray->Direction, box->Transform);
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
    if (d.x < -EPSILON) {
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
    } else if (d.x > EPSILON) {
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
    if (d.y < -EPSILON) {
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
    } else if (d.y > EPSILON) {
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
    if (d.z < -EPSILON) {
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
    } else if (d.z > EPSILON) {
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
insideBox(Vector3D *testPoint, SimpleBody *object)
{
    Vector3D newPoint;
    Box *box = (Box *)object;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        MInverseTransformVector(&newPoint, testPoint, box->Transform);
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
boxNormal(Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    Vector3D newPoint;
    Box *box = (Box *)object;

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        MInverseTransformVector(&newPoint, intersectionPoint, box->Transform);
    } else {
        newPoint.x = intersectionPoint->x;
        newPoint.y = intersectionPoint->y;
        newPoint.z = intersectionPoint->z;
    }

    result->x = 0.0;
    result->y = 0.0;
    result->z = 0.0;
    if (close(newPoint.x, box->bounds[1].x)) {
        result->x = 1.0;
    } else if (close(newPoint.x, box->bounds[0].x)) {
        result->x = -1.0;
    } else if (close(newPoint.y, box->bounds[1].y)) {
        result->y = 1.0;
    } else if (close(newPoint.y, box->bounds[0].y)) {
        result->y = -1.0;
    } else if (close(newPoint.z, box->bounds[1].z)) {
        result->z = 1.0;
    } else if (close(newPoint.z, box->bounds[0].z)) {
        result->z = -1.0;
    } else {
        /* Bad result, should we do something with it? */
        result->x = 1.0;
    }

    /* Transform the point into the boxes space */
    if (box->Transform != nullptr) {
        MTransNormal(result, result, box->Transform);
        VNormalize(*result, *result);
    }
}

void *
copyBox(SimpleBody *object)
{
    Box *newShape;
    Transformation *tr;

    newShape = getBoxShape();
    *newShape = *((Box *)object);
    newShape->Next_Object = nullptr;

    /* Copy any associated transformation */
    if (newShape->Transform != nullptr) {
        tr = getTransformation();
        memcpy(tr, newShape->Transform, sizeof(Transformation));
        newShape->Transform = tr;
    }

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
translateBox(SimpleBody *object, Vector3D *vector)
{
    Transformation transform;
    Box *box = (Box *)object;
    if (box->Transform == nullptr) {
        box->Transform = getTransformation();
    }
    getTranslationTransformation(&transform, vector);
    composeTransformations(box->Transform, &transform);

    translateTexture(&((Box *)object)->Shape_Texture, vector);
}

void
rotateBox(SimpleBody *object, Vector3D *vector)
{
    Transformation transform;
    Box *box = (Box *)object;
    if (box->Transform == nullptr) {
        box->Transform = getTransformation();
    }
    getRotationTransformation(&transform, vector);
    composeTransformations(box->Transform, &transform);

    rotateTexture(&((Box *)object)->Shape_Texture, vector);
}

void
scaleBox(SimpleBody *object, Vector3D *vector)
{
    Transformation transform;
    Box *box = (Box *)object;
    if (box->Transform == nullptr) {
        box->Transform = getTransformation();
    }
    getScalingTransformation(&transform, vector);
    composeTransformations(box->Transform, &transform);

    scaleTexture(&((Box *)object)->Shape_Texture, vector);
}

void
invertBox(SimpleBody *object)
{
    ((Box *)object)->Inverted = 1 - ((Box *)object)->Inverted;
}
