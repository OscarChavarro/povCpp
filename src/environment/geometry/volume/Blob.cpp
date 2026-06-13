/**
This module contains the code for the blob shape.

This file was written by Alexander Enzmann.  He wrote the code for
blobs and generously provided us these enhancements.
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "numericalAnalysis/polynomial/PolynomialSolver.h"
#include "numericalAnalysis/polynomial/QuadraticSolver.h"
#include "numericalAnalysis/polynomial/QuarticSolver.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/material/MaterialUtils.h"

Methods Blob::methodTable = {Blob::allBlobIntersections,
    Blob::insideBlob, Blob::blobNormal, Blob::copyBlob, Blob::translateBlob,
    Blob::rotateBlob, Blob::scaleBlob, Blob::invertBlob};


static constexpr double COEFF_LIMIT = 1.0e-20;
static constexpr double INSIDE_TOLERANCE = 1.0e-6;
static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;
/**
Starting with the density function: (1-r^2)^2, we have a field
that varies in strength from 1 at r = 0 to 0 at r = 1.  By
substituting r/rad for r, we can adjust the range of influence
of a particular component.  By multiplication by coeff, we can
adjust the amount of total contribution, giving the formula:
    coeff * (1 - (r/rad)^2)^2
This varies in strength from coeff at r = 0, to 0 at r = rad.
*/
void
Blob::makeBlob(SimpleBody *obj, double threshold, BlobList *bloblist, int npoints,
    int sflag)
{
    Blob *blob = (Blob *)obj;
    int i;
    double rad;
    double coeff;
    BlobList *temp;

    if (npoints < 1) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Need at least one component in a blob\n");
    }
    blob->threshold = threshold;
    blob->list = new BlobElement[npoints];
    if (blob->list == nullptr) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Failed to allocate blob data\n");
    }
    blob->count = npoints;
    blob->sturmFlag = sflag;

    // Initialize the blob data
    for (i = 0; i < npoints; i++) {
        temp = bloblist;
        if (java::Math::abs(temp->elem.coeffs[2]) < Config::INTERSECTION_EPSILON ||
            temp->elem.radius2 < Config::INTERSECTION_EPSILON) {
            perror("Degenerate blob element\n");
        }
        // Store blob specific information
        rad = temp->elem.radius2;
        rad *= rad;
        coeff = temp->elem.coeffs[2];
        blob->list[i].radius2 = rad;
        blob->list[i].coeffs[2] = coeff;
        blob->list[i].coeffs[1] = -(2.0 * coeff) / rad;
        blob->list[i].coeffs[0] = coeff / (rad * rad);
        blob->list[i].pos = Vector3Dd(
            temp->elem.pos.x(), temp->elem.pos.y(), temp->elem.pos.z());

        bloblist = bloblist->next;
        delete temp;
    }

    // Allocate memory for intersection intervals
    npoints *= 2;
    blob->intervals = new BlobInterval[npoints];
    if (blob->intervals == nullptr) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Failed to allocate blob data\n");
    }
}

/**
Make a sorted list of points along the ray that the various blob
components start and stop adding their influence.  It would take
a very complex blob (with many components along the current ray)
to warrant the overhead of using a faster sort technique.
*/
int
Blob::determineInfluences(
    const Vector3Dd *p, const Vector3Dd *d, const Blob *blob, double mindist)
{
    int i;
    int j;
    int k;
    int cnt;
    double b;
    double t;
    double t0;
    double t1;
    double disc;
    Vector3Dd v;
    BlobInterval * const intervals = blob->intervals;

    cnt = 0;
    for (i = 0; i < blob->count; i++) {
        // Use standard sphere intersection routine
        // to determine where the ray hits the volume
        // of influence of each component of the blob
        v = blob->list[i].pos.subtract(*p);
        b = v.dotProduct(*d);
        t = v.dotProduct(v);
        disc = b * b - t + blob->list[i].radius2;
        if (disc < Config::INTERSECTION_EPSILON) {
            continue;
        }
        disc = java::Math::sqrt(disc);
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

        /**
        Store the points of intersection of this
        blob with the ray.  Keep track of: whether
        this is the start or end point of the hit,
        which component was pierced by the ray,
        and the point along the ray that the
        hit occured at.
        */
        for (k = 0; k < cnt && t0 > intervals[k].bound; k++) {
            ;
        }
        if (k < cnt) {
            // This hit point is smaller than one that
            // already exists - bump the rest and insert
            // it here
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
            // Just plop the start and end points at
            // the end of the list
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

/**
Calculate the field value of a blob - the position vector
"Pos" must already have been transformed into blob space.
*/
double
Blob::calculateFieldValue(SimpleBody *obj, const Vector3Dd *pos)
{
    int i;
    double len;
    double density;
    Vector3Dd v;
    BlobElement *ptr;
    const Blob *blob = (Blob *)obj;

    density = 0.0;
    for (i = 0, ptr = &(blob->list[0]); i < blob->count; i++, ptr++) {
        v = ptr->pos.subtract(*pos);
        len = v.dotProduct(v);
        if (len < ptr->radius2) {
            // Inside the radius of influence of this
            // component, add it's contribution
            density +=
                len * (len * ptr->coeffs[0] + ptr->coeffs[1]) + ptr->coeffs[2];
        }
    }
    return density;
}

// See if the hit in question really is a hit
int
Blob::validateHit(const Blob *blob, const Vector3Dd *p)
{
    int i;
    BlobElement *temp;
    double val;
    double dist;
    Vector3Dd v;
    Vector3Dd n;

    n = Vector3Dd(0.0, 0.0, 0.0);
    temp = &(blob->list[0]);
    for (i = 0; i < blob->count; i++, temp++) {
        v = p->subtract(temp->pos);
        dist = v.dotProduct(v);
        if (dist <= temp->radius2) {
            val = -2.0 * (2.0 * temp->coeffs[0] * dist + temp->coeffs[1]);
            n = Vector3Dd(n.x() + val * v.x(), n.y() + val * v.y(),
                n.z() + val * v.z());
        }
    }
    val = n.dotProduct(n);
    if (val < Config::INTERSECTION_EPSILON) {
        return 0;
    }
    return 1;
}

/**
Generate intervals of influence of each component.  After these
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
    Blob * const blob = (Blob *)object;
    Intersection localElement;
    double dist;
    double len;
    double *tcoeffs;
    double coeffs[5];
    double roots[4];
    int i;
    int j;
    int cnt;
    Vector3Dd p;
    Vector3Dd d;
    Vector3Dd v;
    int rootCount;
    int inFlag;
    BlobElement *element;
    double t0;
    double t1;
    double c0;
    double c1;
    double c2;
    Vector3Dd intersectionPoint;
    Vector3Dd dv;
    const BlobInterval *intervals = blob->intervals;
    bool intersectionFound = false;

    Statistics::global().rayBlobTests++;

    // Transform the ray into the blob space
    if (blob->transformation != nullptr) {
        p = blob->transformationInverse->transformPoint(ray->position);
        d = blob->transformationInverse->transformDirection(ray->direction);
    } else {
        p = Vector3Dd(ray->position.x(), ray->position.y(), ray->position.z());
        d = Vector3Dd(ray->direction.x(), ray->direction.y(), ray->direction.z());
    }

    len = java::Math::sqrt(d.x() * d.x() + d.y() * d.y() + d.z() * d.z());
    if (len == 0.0) {
        return 0;
    }
    d = Vector3Dd(d.x() / len, d.y() / len, d.z() / len);

    // Figure out the intervals along the ray where each
    // component of the blob has an effect.
    if ((cnt = Blob::determineInfluences(&p, &d, blob, 0.01)) == 0) {
        // Ray doesn't hit the sphere of influence of any of
        // its component elements
        return 0;
    }

    // Clear out the coefficients
    for (i = 0; i < 4; i++) {
        coeffs[i] = 0.0;
    }
    coeffs[4] = -blob->threshold;

    // Step through the list of influence points, adding the
    // influence of each blob component as it appears
    for (i = 0, inFlag = 0; i < cnt; i++) {
        if (intervals[i].type == 0) {
            // Something is just starting to influence the ray,
            // so calculate its coefficients and add them
            // into the pot.
            inFlag++;
            element = blob->list + intervals[i].index;

            v = p.subtract(element->pos);
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
            // We are losing the influence of a component, so
            // subtract off its coefficients
            tcoeffs = &(blob->list[intervals[i].index].tcoeffs[0]);
            for (j = 0; j < 5; j++) {
                coeffs[j] -= tcoeffs[j];
            }
            if (--inFlag == 0) {
                // None of the components are currently affecting
                // the ray - skip ahead.
                continue;
            }
        }

        // Figure out which root solver to use
        if (blob->sturmFlag == 0) {
            // Use Ferrari's method
            rootCount = QuarticSolver::solve(coeffs, &roots[0],
                ray->isShadowRay ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                Config::POLYNOMIAL_SOLVER_EPSILON);
        } else
            // Sturm sequences
            if (java::Math::abs(coeffs[0]) < COEFF_LIMIT) {
                if (java::Math::abs(coeffs[1]) < COEFF_LIMIT) {
                    rootCount = QuadraticSolver::solve(&coeffs[2], &roots[0]);
                } else {
                    rootCount =
                        PolynomialSolver::solvePolynomial(3, &coeffs[1], &roots[0],
                            ray->isShadowRay ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                            Config::POLYNOMIAL_SOLVER_EPSILON);
                }
            } else {
                rootCount = PolynomialSolver::solvePolynomial(4, coeffs, &roots[0],
                    ray->isShadowRay ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                    Config::POLYNOMIAL_SOLVER_EPSILON);
            }

        // See if any of the roots are valid
        for (j = 0; j < rootCount; j++) {
            dist = roots[j];
            // First see if the root is in the interval of influence of
            // the currently active components of the blob
            if ((dist >= intervals[i].bound) &&
                (dist <= intervals[i + 1].bound)) {
                intersectionPoint = d.multiply(dist);
                intersectionPoint = intersectionPoint.add(p);
                if (true || Blob::validateHit(blob, &intersectionPoint)) {
                    // Only add this hit if it really is near the surface, we
                    // can get fooled by numerical inaccuracies
                    // Transform the point into world space
                    if (blob->transformation != nullptr) {
                        intersectionPoint = blob->transformation->transformPoint(
                            intersectionPoint);
                    }
                    dv = intersectionPoint.subtract(ray->position);
                    len = dv.length();
                    localElement.Depth = len;
                    localElement.Object = nullptr;
                    localElement.Point = intersectionPoint;
                    localElement.Shape = (Geometry *)blob;
                    depthQueue->add(&localElement);
                    intersectionFound = true;
                }
            }
        }
    }
    if (intersectionFound) {
        Statistics::global().rayBlobTestsSucceeded++;
    }
    return intersectionFound;
}

/**
Calculate the density at this point, then compare to
the threshold to see if we are in or out of the blob
*/
int
Blob::insideBlob(Vector3Dd *testPoint, SimpleBody *object)
{
    Vector3Dd newPoint;
    const Blob *blob = (Blob *)object;

    // Transform the point into blob space
    if (blob->transformation != nullptr) {
        newPoint = blob->transformationInverse->transformPoint(*testPoint);
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
    double dist;
    double val;
    const Blob *blob = (Blob *)object;
    BlobElement *temp;

    // Transform the point into the blobs space
    if (blob->transformation != nullptr) {
        newPoint = blob->transformationInverse->transformPoint(*intersectionPoint);
    } else {
        newPoint = Vector3Dd(
            intersectionPoint->x(), intersectionPoint->y(), intersectionPoint->z());
    }

    *result = Vector3Dd(0.0, 0.0, 0.0);

    // For each component that contributes to this point, add
    // its bit to the normal
    temp = &(blob->list[0]);
    for (i = 0; i < blob->count; i++, temp++) {
        v = Vector3Dd(newPoint.x() - temp->pos.x(),
            newPoint.y() - temp->pos.y(), newPoint.z() - temp->pos.z());
        dist = (v.x() * v.x() + v.y() * v.y() + v.z() * v.z());

        if (dist <= temp->radius2) {
            val = -2.0 * (2.0 * temp->coeffs[0] * dist + temp->coeffs[1]);
            *result = Vector3Dd(result->x() + val * v.x(),
                result->y() + val * v.y(), result->z() + val * v.z());
        }
    }
    val = (result->x() * result->x() + result->y() * result->y() +
           result->z() * result->z());
    if (val < Config::INTERSECTION_EPSILON) {
        *result = Vector3Dd(1.0, 0.0, 0.0);
    } else {
        val = 1.0 / java::Math::sqrt(val);
        *result = (*result).multiply(val);
    }

    // Transform back to world space
    if (blob->transformation != nullptr) {
        *result = blob->transformationInverse->withoutTranslation().multiply(*result);
    }
    *result = (*result).normalizedFast();
}

void *
Blob::copyBlob(SimpleBody *object)
{
    Blob *blob;
    const Blob *oldShape = (Blob *)object;

    blob = new Blob;
    memcpy(blob, oldShape, sizeof(Blob));
    blob->nextObject = nullptr;

    // Allocate space and copy the blob specific data
    blob->list = new BlobElement[oldShape->count];
    if (blob->list == nullptr) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Failed to allocate blob data\n");
    }
    for (int i = 0; i < oldShape->count; i++) {
        blob->list[i] = oldShape->list[i];
    }
    blob->intervals = new BlobInterval[2 * blob->count];
    if (blob->intervals == nullptr) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Failed to allocate blob data\n");
    }

    // Copy any associated transformation
    if (blob->transformation != nullptr) {
        blob->transformation = new Matrix4x4d(*(blob->transformation));
        blob->transformationInverse = new Matrix4x4d(*(blob->transformationInverse));
    }

    // Copy any associated texture
    if (blob->material != nullptr) {
        blob->material = MaterialUtils::instance().copyTexture(blob->material);
    }

    return (blob);
}

void
Blob::translateBlob(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Blob * const blob = (Blob *)object;
    if (blob->transformation == nullptr) {
        blob->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        blob->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    deltaTransformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *blob->transformation = blob->transformation->multiply(deltaTransformation);
    *blob->transformationInverse =
        deltaTransformationInverse.multiply(*blob->transformationInverse);

    MaterialUtils::instance().translateTexture(&((Blob *)object)->material, vector);
}

void
Blob::rotateBlob(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Blob * const blob = (Blob *)object;
    if (blob->transformation == nullptr) {
        blob->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        blob->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *blob->transformation = blob->transformation->multiply(deltaTransformation);
    *blob->transformationInverse =
        deltaTransformationInverse.multiply(*blob->transformationInverse);

    MaterialUtils::instance().rotateTexture(&((Blob *)object)->material, vector);
}

void
Blob::scaleBlob(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Blob * const blob = (Blob *)object;
    if (blob->transformation == nullptr) {
        blob->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        blob->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    deltaTransformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *blob->transformation = blob->transformation->multiply(deltaTransformation);
    *blob->transformationInverse =
        deltaTransformationInverse.multiply(*blob->transformationInverse);

    MaterialUtils::instance().scaleTexture(&((Blob *)object)->material, vector);
}

void
Blob::invertBlob(SimpleBody *object)
{
    ((Blob *)object)->Inverted = !((Blob *)object)->Inverted;
}
