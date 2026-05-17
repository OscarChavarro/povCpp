/****************************************************************************
 *                     blob.c
 *
 *  This module contains the code for the blob shape.
 *
 *  This file was written by Alexander Enzmann.  He wrote the code for
 *  blobs and generously provided us these enhancements.
 *
 *****************************************************************************/

#include "environment/geometry/volume/Blob.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "io/Parse.h"
#include "processing/PolynomialSolver.h"
Methods Blob_Methods = {Composite::objectIntersect, Blob::allBlobIntersections,
    Blob::insideBlob, Blob::blobNormal, Blob::copyBlob, Blob::translateBlob,
    Blob::rotateBlob, Blob::scaleBlob, Blob::invertBlob};

extern RayWithSegments *vpRay;
extern long rayBlobTests, rayBlobTestsSucceeded;

static constexpr double COEFF_LIMIT = 1.0e-20;
static constexpr double INSIDE_TOLERANCE = 1.0e-6;
/* Starting with the density function: (1-r^2)^2, we have a field
    that varies in strength from 1 at r = 0 to 0 at r = 1.  By
    substituting r/rad for r, we can adjust the range of influence
    of a particular component.  By multiplication by coeff, we can
    adjust the amount of total contribution, giving the formula:
        coeff * (1 - (r/rad)^2)^2
    This varies in strength from coeff at r = 0, to 0 at r = rad. */
void
MakeBlob(SimpleBody *obj, double threshold, BlobList *bloblist, int npoints,
    int sflag)
{
    Blob *blob = (Blob *)obj;
    int i;
    double rad, coeff;
    BlobList *temp;

    if (npoints < 1) {
        printf("Need at least one component in a blob\n");
        exit(1);
    }
    blob->threshold = threshold;
    blob->list = new BlobElement[npoints];
    if (blob->list == nullptr) {
        printf("Failed to allocate blob data\n");
        exit(1);
    }
    blob->count = npoints;
    blob->Sturm_Flag = sflag;

    /* Initialize the blob data */
    for (i = 0; i < npoints; i++) {
        temp = bloblist;
        if (fabs(temp->elem.coeffs[2]) < kEpsilon ||
            temp->elem.radius2 < kEpsilon) {
            perror("Degenerate blob element\n");
        }
        /* Store blob specific information */
        rad = temp->elem.radius2;
        rad *= rad;
        coeff = temp->elem.coeffs[2];
        blob->list[i].radius2 = rad;
        blob->list[i].coeffs[2] = coeff;
        blob->list[i].coeffs[1] = -(2.0 * coeff) / rad;
        blob->list[i].coeffs[0] = coeff / (rad * rad);
        blob->list[i].pos.x = temp->elem.pos.x;
        blob->list[i].pos.y = temp->elem.pos.y;
        blob->list[i].pos.z = temp->elem.pos.z;

        bloblist = bloblist->next;
        delete temp;
    }

    /*  Allocate memory for intersection intervals */
    npoints *= 2;
    blob->intervals = new BlobInterval[npoints];
    if (blob->intervals == nullptr) {
        printf("Failed to allocate blob data\n");
        exit(1);
    }
}

/* Make a sorted list of points along the ray that the various blob
    components start and stop adding their influence.  It would take
    a very complex blob (with many components along the current ray)
    to warrant the overhead of using a faster sort technique. */
int
Blob::determineInfluences(
    Vector3Dd *p, Vector3Dd *d, Blob *blob, double mindist)
{
    int i;
    int j;
    int k;
    int cnt;
    double b, t, t0, t1, disc;
    Vector3Dd v;
    BlobInterval *intervals = blob->intervals;

    cnt = 0;
    for (i = 0; i < blob->count; i++) {
        /* Use standard sphere intersection routine
            to determine where the ray hits the volume
            of influence of each component of the blob. */
        VectorOps::vSub(v, blob->list[i].pos, *p);
        b = v.dotProduct(*d);
        t = v.dotProduct(v);
        disc = b * b - t + blob->list[i].radius2;
        if (disc < kEpsilon) {
            continue;
        }
        disc = sqrt(disc);
        t1 = b + disc;
        if (t1 < mindist) {
            t1 = 0.0;
        }
        t0 = b - disc;
        if (t0 < mindist) {
            t0 = 0.0;
        }
        if (t1 == t0) {
            continue;
        }
        if (t1 < t0) {
            disc = t0;
            t0 = t1;
            t1 = disc;
        }

        /* Store the points of intersection of this
            blob with the ray.  Keep track of: whether
            this is the start or end point of the hit,
            which component was pierced by the ray,
            and the point along the ray that the
            hit occured at. */
        for (k = 0; k < cnt && t0 > intervals[k].bound; k++) {
            ;
        }
        if (k < cnt) {
            /* This hit point is smaller than one that
                already exists - bump the rest and insert
                it here */
            for (j = cnt; j > k; j--) {
                memcpy(&intervals[j], &intervals[j - 1], sizeof(BlobInterval));
            }
            intervals[k].type = 0;
            intervals[k].index = i;
            intervals[k].bound = t0;
            cnt++;
            for (k = k + 1; k < cnt && t1 > intervals[k].bound; k++) {
                ;
            }
            if (k < cnt) {
                for (j = cnt; j > k; j--) {
                    memcpy(
                        &intervals[j], &intervals[j - 1], sizeof(BlobInterval));
                }
                intervals[k].type = 1;
                intervals[k].index = i;
                intervals[k].bound = t1;
            } else {
                intervals[cnt].type = 1;
                intervals[cnt].index = i;
                intervals[cnt].bound = t1;
            }
            cnt++;
        } else {
            /* Just plop the start and end points at
                the end of the list */
            intervals[cnt].type = 0;
            intervals[cnt].index = i;
            intervals[cnt].bound = t0;
            cnt++;
            intervals[cnt].type = 1;
            intervals[cnt].index = i;
            intervals[cnt].bound = t1;
            cnt++;
        }
    }
    return cnt;
}

/* Calculate the field value of a blob - the position vector
    "Pos" must already have been transformed into blob space. */
double
Blob::calculateFieldValue(SimpleBody *obj, Vector3Dd *pos)
{
    int i;
    double len, density;
    Vector3Dd v;
    BlobElement *ptr;
    Blob *blob = (Blob *)obj;

    density = 0.0;
    for (i = 0, ptr = &(blob->list[0]); i < blob->count; i++, ptr++) {
        VectorOps::vSub(v, ptr->pos, *pos);
        len = v.dotProduct(v);
        if (len < ptr->radius2) {
            /* Inside the radius of influence of this
                component, add it's contribution */
            density +=
                len * (len * ptr->coeffs[0] + ptr->coeffs[1]) + ptr->coeffs[2];
        }
    }
    return density;
}

/* See if the hit in question really is a hit. */
int
Blob::validateHit(Blob *blob, Vector3Dd *p)
{
    int i;
    BlobElement *temp;
    double val, dist;
    Vector3Dd v;
    Vector3Dd n;

    n.x = 0.0;
    n.y = 0.0;
    n.z = 0.0;
    temp = &(blob->list[0]);
    for (i = 0; i < blob->count; i++, temp++) {
        VectorOps::vSub(v, *p, temp->pos);
        dist = v.dotProduct(v);
        if (dist <= temp->radius2) {
            val = -2.0 * (2.0 * temp->coeffs[0] * dist + temp->coeffs[1]);
            n.x += val * v.x;
            n.y += val * v.y;
            n.z += val * v.z;
        }
    }
    val = n.dotProduct(n);
    if (val < kEpsilon) {
        return 0;
    }
    return 1;
}

/* Generate intervals of influence of each component.  After these
    are made, determine their aggregate effect on the ray.  As the
    individual intervals are checked, a quartic is generated
    that represents the density at a particular point on the ray.

    After making the substitutions in MakeBlob, there is a formula
    for each component that has the form:

        c0 * r^4 + c1 * r^2 + c2.

    In order to determine the influence on the ray of all of the
    individual components, we start by determining the distance
    from any point on the ray to the specified point.  This can
    be found using the pythagorean theorem, using C as the center
    of this component, P as the start of the ray, and D as the
    direction of travel of the ray:

        r^2 = (t * D + P - C) . (t * D + P - C)

    we insert this equation for each appearance of r^2 in the
    components' formula, giving:

        r^2 = D.D t^2 + 2 t D . (P - C) + (P - C) . (P - C)

    Since the direction vector has been normalized, D.D = 1.
    Using the substitutions:

        t0 = (P - C) . (P - C),
        t1 = D . (P - C)

    We can write the formula as:

        r^2 = t0 + 2 t t1 + t^2

    Taking r^2 and substituting into the formula for this component
    of the blob we get the formula:

        density = c0 * (r^2)^2 + c1 * r^2 + c2,

    or:

        density = c0 * (t0 + 2 t t1 + t^2)^2 +
                     c1 * (t0 + 2 t t1 + t^2) +
                     c2

    Expanding terms and collecting with respect to "t" gives:
        t^4 * c0 +
        t^3 * 4 c0 t1 +
        t^2 * (c1 + 2 * c0 t0 + 4 c0 t1^2)
        t    * 2 (c1 t1 + 2 c0 t0 t1) +
                c2 + c1*t0 + c0*t0^2

    This formula can now be solved for "t" by any of the quartic
    root solvers that are available.
*/
int
Blob::allBlobIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    Blob *blob = (Blob *)object;
    Intersection localElement;
    double dist, len, *tcoeffs, coeffs[5], roots[4];
    int i;
    int j;
    int cnt;
    Vector3Dd p;
    Vector3Dd d;
    Vector3Dd v;
    int rootCount;
    int inFlag;
    BlobElement *element;
    double t0, t1, c0, c1, c2;
    Vector3Dd intersectionPoint;
    Vector3Dd dv;
    BlobInterval *intervals = blob->intervals;
    int intersectionFound = FALSE;

    rayBlobTests++;

    /* Transform the ray into the blob space */
    if (blob->Transform != nullptr) {
        Transformation::MInverseTransformVector(
            &p, &ray->position, blob->Transform);
        Transformation::MInvTransVector(&d, &ray->direction, blob->Transform);
    } else {
        p.x = ray->position.x;
        p.y = ray->position.y;
        p.z = ray->position.z;
        d.x = ray->direction.x;
        d.y = ray->direction.y;
        d.z = ray->direction.z;
    }

    len = sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len == 0.0) {
        return 0;
    }
    d.x /= len;
    d.y /= len;
    d.z /= len;

    /* Figure out the intervals along the ray where each
    component of the blob has an effect. */
    if ((cnt = Blob::determineInfluences(&p, &d, blob, 0.01)) == 0) {
        /* Ray doesn't hit the sphere of influence of any of
        its component elements */
        return 0;
    }

    /* Clear out the coefficients */
    for (i = 0; i < 4; i++) {
        coeffs[i] = 0.0;
    }
    coeffs[4] = -blob->threshold;

    /* Step through the list of influence points, adding the
        influence of each blob component as it appears */
    for (i = 0, inFlag = 0; i < cnt; i++) {
        if (intervals[i].type == 0) {
            /* Something is just starting to influence the ray,
                so calculate its coefficients and add them
            into the pot. */
            inFlag++;
            element = blob->list + intervals[i].index;

            VectorOps::vSub(v, p, element->pos);
            c0 = element->coeffs[0];
            c1 = element->coeffs[1];
            c2 = element->coeffs[2];
            t0 = v.dotProduct(v);
            t1 = v.dotProduct(d);
            tcoeffs = &(element->tcoeffs[0]);

            tcoeffs[0] = c0;
            tcoeffs[1] = 4.0 * c0 * t1;
            tcoeffs[2] = 2.0 * c0 * (2.0 * t1 * t1 + t0) + c1;
            tcoeffs[3] = 2.0 * t1 * (2.0 * c0 * t0 + c1);
            tcoeffs[4] = c0 * t0 * t0 + c1 * t0 + c2;

            for (j = 0; j < 5; j++) {
                coeffs[j] += tcoeffs[j];
            }
        } else {
            /* We are losing the influence of a component, so
                subtract off its coefficients */
            tcoeffs = &(blob->list[intervals[i].index].tcoeffs[0]);
            for (j = 0; j < 5; j++) {
                coeffs[j] -= tcoeffs[j];
            }
            if (--inFlag == 0) {
                /* None of the components are currently affecting
                    the ray - skip ahead. */
                continue;
            }
        }

        /* Figure out which root solver to use */
        if (blob->Sturm_Flag == 0) {
            /* Use Ferrari's method */
            rootCount = PolynomialSolver::solveQuartic(coeffs, &roots[0]);
        } else
            /* Sturm sequences */
            if (fabs(coeffs[0]) < COEFF_LIMIT) {
                if (fabs(coeffs[1]) < COEFF_LIMIT) {
                    rootCount =
                        PolynomialSolver::solveQuadratic(&coeffs[2], &roots[0]);
                } else {
                    rootCount =
                        PolynomialSolver::polysolve(3, &coeffs[1], &roots[0]);
                }
            } else {
                rootCount = PolynomialSolver::polysolve(4, coeffs, &roots[0]);
            }

        /* See if any of the roots are valid */
        for (j = 0; j < rootCount; j++) {
            dist = roots[j];
            /* First see if the root is in the interval of influence of
                the currently active components of the blob */
            if ((dist >= intervals[i].bound) &&
                (dist <= intervals[i + 1].bound)) {
                VectorOps::vScale(intersectionPoint, d, dist);
                intersectionPoint.add(p);
                if (true || Blob::validateHit(blob, &intersectionPoint)) {
                    /* Only add this hit if it really is near the surface, we
                       can get fooled by numerical inaccuracies */
                    /* Transform the point into world space */
                    if (blob->Transform != nullptr) {
                        Transformation::MTransformVector(&intersectionPoint,
                            &intersectionPoint, blob->Transform);
                    }
                    VectorOps::vSub(dv, intersectionPoint, ray->position);
                    len = dv.length();
                    localElement.Depth = len;
                    localElement.Object = blob->Parent_Object;
                    localElement.Point = intersectionPoint;
                    localElement.Shape = (Geometry *)blob;
                    depthQueue->add(&localElement);
                    intersectionFound = TRUE;
                }
            }
        }
    }
    if (intersectionFound) {
        rayBlobTestsSucceeded++;
    }
    return intersectionFound;
}

/* Calculate the density at this point, then compare to
    the threshold to see if we are in or out of the blob */
int
Blob::insideBlob(Vector3Dd *testPoint, SimpleBody *object)
{
    Vector3Dd newPoint;
    Blob *blob = (Blob *)object;

    /* Transform the point into blob space */
    if (blob->Transform != nullptr) {
        Transformation::MInverseTransformVector(
            &newPoint, testPoint, blob->Transform);
    } else {
        newPoint = *testPoint;
    }

    if (Blob::calculateFieldValue(object, &newPoint) >
        blob->threshold - INSIDE_TOLERANCE) {
        return ((int)1 - blob->Inverted);
    }
    return ((int)blob->Inverted);
}

void
Blob::blobNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    Vector3Dd newPoint;
    Vector3Dd v;
    int i;
    double dist, val;
    Blob *blob = (Blob *)object;
    BlobElement *temp;

    /* Transform the point into the blobs space */
    if (blob->Transform != nullptr) {
        Transformation::MInverseTransformVector(
            &newPoint, intersectionPoint, blob->Transform);
    } else {
        newPoint.x = intersectionPoint->x;
        newPoint.y = intersectionPoint->y;
        newPoint.z = intersectionPoint->z;
    }

    result->x = 0.0;
    result->y = 0.0;
    result->z = 0.0;

    /* For each component that contributes to this point, add
        its bit to the normal */
    temp = &(blob->list[0]);
    for (i = 0; i < blob->count; i++, temp++) {
        v.x = newPoint.x - temp->pos.x;
        v.y = newPoint.y - temp->pos.y;
        v.z = newPoint.z - temp->pos.z;
        dist = (v.x * v.x + v.y * v.y + v.z * v.z);

        if (dist <= temp->radius2) {
            val = -2.0 * (2.0 * temp->coeffs[0] * dist + temp->coeffs[1]);
            result->x += val * v.x;
            result->y += val * v.y;
            result->z += val * v.z;
        }
    }
    val =
        (result->x * result->x + result->y * result->y + result->z * result->z);
    if (val < kEpsilon) {
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
    } else {
        val = 1.0 / sqrt(val);
        result->x *= val;
        result->y *= val;
        result->z *= val;
    }

    /* Transform back to world space */
    if (blob->Transform != nullptr) {
        Transformation::MTransNormal(result, result, blob->Transform);
    }
    (*result).normalize();
}

void *
Blob::copyBlob(SimpleBody *object)
{
    Blob *blob;
    Blob *oldShape = (Blob *)object;
    Transformation *tr;

    blob = SceneFactory::getBlobShape();
    memcpy(blob, oldShape, sizeof(Blob));
    blob->Next_Object = nullptr;

    /* Allocate space and copy the blob specific data */
    blob->list = new BlobElement[oldShape->count];
    if (blob->list == nullptr) {
        printf("Failed to allocate blob data\n");
        exit(1);
    }
    memcpy(blob->list, oldShape->list, oldShape->count * sizeof(BlobElement));
    blob->intervals = new BlobInterval[2 * blob->count];
    if (blob->intervals == nullptr) {
        printf("Failed to allocate blob data\n");
        exit(1);
    }

    /* Copy any associated transformation */
    if (blob->Transform != nullptr) {
        tr = Transformation::getTransformation();
        memcpy(tr, blob->Transform, sizeof(Transformation));
        blob->Transform = tr;
    }

    /* Copy any associated texture */
    if (blob->Shape_Texture != nullptr) {
        blob->Shape_Texture = TextureParser::copyTexture(blob->Shape_Texture);
    }

    return (blob);
}

void
Blob::translateBlob(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transform;
    Blob *blob = (Blob *)object;
    if (blob->Transform == nullptr) {
        blob->Transform = Transformation::getTransformation();
    }
    Transformation::getTranslationTransformation(&transform, vector);
    Transformation::composeTransformations(blob->Transform, &transform);

    TextureUtils::translateTexture(&((Blob *)object)->Shape_Texture, vector);
}

void
Blob::rotateBlob(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transform;
    Blob *blob = (Blob *)object;
    if (blob->Transform == nullptr) {
        blob->Transform = Transformation::getTransformation();
    }
    Transformation::getRotationTransformation(&transform, vector);
    Transformation::composeTransformations(blob->Transform, &transform);

    TextureUtils::rotateTexture(&((Blob *)object)->Shape_Texture, vector);
}

void
Blob::scaleBlob(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transform;
    Blob *blob = (Blob *)object;
    if (blob->Transform == nullptr) {
        blob->Transform = Transformation::getTransformation();
    }
    Transformation::getScalingTransformation(&transform, vector);
    Transformation::composeTransformations(blob->Transform, &transform);

    TextureUtils::scaleTexture(&((Blob *)object)->Shape_Texture, vector);
}

void
Blob::invertBlob(SimpleBody *object)
{
    ((Blob *)object)->Inverted = 1 - ((Blob *)object)->Inverted;
}
