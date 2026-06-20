#include <cstdio>

/**
This module contains the code for the blob shape.

This file was written by Alexander Enzmann.  He wrote the code for
blobs and generously provided us these enhancements.
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/QuadraticSolver.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/QuarticSolver.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/volume/Blob.h"

static constexpr double COEFF_LIMIT = 1.0e-20;
static constexpr double INSIDE_TOLERANCE = 1.0e-6;
static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;

static BlobElement *
allocateBlobElements(int count)
{
    if (count < 1) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Need at least one component in a blob\n");
    }

    BlobElement *elements = new BlobElement[count];
    if (elements == nullptr) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Failed to allocate blob data\n");
    }
    return elements;
}

static BlobInterval *
allocateBlobIntervals(int count)
{
    if (count < 1) {
        return nullptr;
    }

    BlobInterval *allocatedIntervals = new BlobInterval[2 * count];
    if (allocatedIntervals == nullptr) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Failed to allocate blob data\n");
    }
    return allocatedIntervals;
}

Blob::Blob(double thresholdValue, BlobList *bloblist, int npoints,
    int sturmFlagValue) :
    transformation(nullptr),
    transformationInverse(nullptr),
    inverted(false),
    count(npoints),
    threshold(thresholdValue),
    list(allocateBlobElements(npoints)),
    intervals(allocateBlobIntervals(npoints)),
    sturmFlag(sturmFlagValue)
{
    for (int i = 0; i < npoints; i++) {
        BlobList *temp = bloblist;
        if (java::Math::abs(temp->getElem().getCoeffs()[2]) < Config::INTERSECTION_EPSILON ||
            temp->getElem().getRadius2() < Config::INTERSECTION_EPSILON) {
            perror("Degenerate blob element\n");
        }
        double rad = temp->getElem().getRadius2();
        rad *= rad;
        double coeff = temp->getElem().getCoeffs()[2];
        list[i].setRadius2(rad);
        list[i].getCoeffs()[2] = coeff;
        list[i].getCoeffs()[1] = -(2.0 * coeff) / rad;
        list[i].getCoeffs()[0] = coeff / (rad * rad);
        list[i].getPos() = Vector3Dd(
            temp->getElem().getPos().x(), temp->getElem().getPos().y(),
            temp->getElem().getPos().z());

        bloblist = bloblist->getNext();
        delete temp;
    }
}

Blob::Blob(const Matrix4x4d *transformationValue,
    const Matrix4x4d *transformationInverseValue, bool invertedValue,
    int countValue, double thresholdValue, const BlobElement *listValue,
    int sturmFlagValue) :
    transformation(nullptr),
    transformationInverse(nullptr),
    inverted(invertedValue),
    count(countValue),
    threshold(thresholdValue),
    list(countValue > 0 ? allocateBlobElements(countValue) : nullptr),
    intervals(allocateBlobIntervals(countValue)),
    sturmFlag(sturmFlagValue)
{
    if (transformationValue != nullptr) {
        transformation = new Matrix4x4d(*transformationValue);
        transformationInverse = transformationInverseValue != nullptr ?
            new Matrix4x4d(*transformationInverseValue) : nullptr;
    }

    if (count <= 0) {
        return;
    }
    for (int i = 0; i < count; i++) {
        list[i] = listValue[i];
    }
}
/**
Starting with the density function: (1-r^2)^2, we have a field
that varies in strength from 1 at r = 0 to 0 at r = 1.  By
substituting r/rad for r, we can adjust the range of influence
of a particular component.  By multiplication by coeff, we can
adjust the amount of total contribution, giving the formula:
    coeff * (1 - (r/rad)^2)^2
This varies in strength from coeff at r = 0, to 0 at r = rad.
*/
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
    int cnt = 0;
    double b;
    double t;
    double t0;
    double t1;
    double disc;
    Vector3Dd v;
    BlobInterval * const intervals = blob->intervals;
    for (i = 0; i < blob->count; i++) {
        // Use standard sphere intersection routine
        // to determine where the ray hits the volume
        // of influence of each component of the blob
        v = blob->list[i].getPos().subtract(*p);
        b = v.dotProduct(*d);
        t = v.dotProduct(v);
        disc = b * b - t + blob->list[i].getRadius2();
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
        for (k = 0; k < cnt && t0 > intervals[k].getBound(); k++) {
            ;
        }
        if (k < cnt) {
            // This hit point is smaller than one that
            // already exists - bump the rest and insert
            // it here
            for (j = cnt; j > k; j--) {
                memcpy(&intervals[j], &intervals[j - 1], sizeof(BlobInterval));
            }
            intervals[k].setType(0);
            intervals[k].setIndex(i);
            intervals[k].setBound(t0);
            cnt++;
            for (k = k + 1; k < cnt && t1 > intervals[k].getBound(); k++) {
                ;
            }
            if (k < cnt) {
                for (j = cnt; j > k; j--) {
                    memcpy(
                        &intervals[j], &intervals[j - 1], sizeof(BlobInterval));
                }
                intervals[k].setType(1);
                intervals[k].setIndex(i);
                intervals[k].setBound(t1);
            } else {
                intervals[cnt].setType(1);
                intervals[cnt].setIndex(i);
                intervals[cnt].setBound(t1);
            }
            cnt++;
        } else {
            // Just plop the start and end points at
            // the end of the list
            intervals[cnt].setType(0);
            intervals[cnt].setIndex(i);
            intervals[cnt].setBound(t0);
            cnt++;
            intervals[cnt].setType(1);
            intervals[cnt].setIndex(i);
            intervals[cnt].setBound(t1);
            cnt++;
        }
    }
    return cnt;
}

/**
Calculate the field value of a blob - the origin vector
"Pos" must already have been transformed into blob space.
*/
double
Blob::calculateFieldValue(BoundedGeometry *obj, const Vector3Dd *pos)
{
    double density = 0.0;
    Vector3Dd v;
    BlobElement *ptr;
    const Blob *blob = (Blob *)obj;
    ptr = &(blob->list[0]);
    for (int i = 0; i < blob->count; i++, ptr++) {
        v = ptr->getPos().subtract(*pos);
        double len = v.dotProduct(v);
        if (len < ptr->getRadius2()) {
            // Inside the radius of influence of this
            // component, add it's contribution
            density +=
                len * (len * ptr->getCoeffs()[0] + ptr->getCoeffs()[1]) +
                ptr->getCoeffs()[2];
        }
    }
    return density;
}

// See if the hit in question really is a hit
int
Blob::validateHit(const Blob *blob, const Vector3Dd *p)
{
    int i;
    BlobElement *temp = &(blob->list[0]);
    double val;
    double dist;
    Vector3Dd v;
    Vector3Dd n = Vector3Dd(0.0, 0.0, 0.0);
    for (i = 0; i < blob->count; i++, temp++) {
        v = p->subtract(temp->getPos());
        dist = v.dotProduct(v);
        if (dist <= temp->getRadius2()) {
            val = -2.0 * (2.0 * temp->getCoeffs()[0] * dist +
                temp->getCoeffs()[1]);
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
Blob::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    Blob * const blob = this;
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
    Statistics &stats = *ray->getStatistics();

    stats.incrementRayBlobTests();

    // Transform the ray into the blob space
    if (blob->transformation != nullptr) {
        p = blob->transformationInverse->transformPoint(ray->getOrigin());
        d = blob->transformationInverse->transformDirection(ray->getDirection());
    } else {
        p = Vector3Dd(ray->getOrigin().x(), ray->getOrigin().y(), ray->getOrigin().z());
        d = Vector3Dd(ray->getDirection().x(), ray->getDirection().y(), ray->getDirection().z());
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
        if (intervals[i].getType() == 0) {
            // Something is just starting to influence the ray,
            // so calculate its coefficients and add them
            // into the pot.
            inFlag++;
            element = blob->list + intervals[i].getIndex();

            v = p.subtract(element->getPos());
            c0 = element->getCoeffs()[0];
            c1 = element->getCoeffs()[1];
            c2 = element->getCoeffs()[2];
            t0 = v.dotProduct(v);
            t1 = v.dotProduct(d);
            tcoeffs = &(element->getTCoeffs()[0]);

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
            tcoeffs = &(blob->list[intervals[i].getIndex()].getTCoeffs()[0]);
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
                ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                Config::POLYNOMIAL_SOLVER_EPSILON);
        } else
            // Sturm sequences
            if (java::Math::abs(coeffs[0]) < COEFF_LIMIT) {
                if (java::Math::abs(coeffs[1]) < COEFF_LIMIT) {
                    rootCount = QuadraticSolver::solve(&coeffs[2], &roots[0]);
                } else {
                    PolynomialSolver cubicSolver(3, &coeffs[1]);
                    rootCount = cubicSolver.solve(&roots[0],
                        ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                        Config::POLYNOMIAL_SOLVER_EPSILON);
                }
            } else {
                PolynomialSolver quarticSolver(4, coeffs);
                rootCount = quarticSolver.solve(&roots[0],
                    ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                    Config::POLYNOMIAL_SOLVER_EPSILON);
            }

        // See if any of the roots are valid
        for (j = 0; j < rootCount; j++) {
            dist = roots[j];
            // First see if the root is in the interval of influence of
            // the currently active components of the blob
            if ((dist >= intervals[i].getBound()) &&
                (dist <= intervals[i + 1].getBound())) {
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
                    dv = intersectionPoint.subtract(ray->getOrigin());
                    len = dv.length();
                    localElement.setDepth(len);
                    localElement.setObject(nullptr);
                    localElement.setPoint(intersectionPoint);
                    localElement.setShape(reinterpret_cast<SimpleBody *>(blob));
                    depthQueue->offer(localElement);
                    intersectionFound = true;
                }
            }
        }
    }
    if (intersectionFound) {
        stats.incrementRayBlobTestsSucceeded();
    }
    return intersectionFound;
}

/**
Calculate the density at this point, then compare to
the threshold to see if we are in or out of the blob
*/
int
Blob::inside(Vector3Dd *testPoint)
{
    Vector3Dd newPoint;
    const Blob *blob = this;

    // Transform the point into blob space
    if (blob->transformation != nullptr) {
        newPoint = blob->transformationInverse->transformPoint(*testPoint);
    } else {
        newPoint = *testPoint;
    }

    if (Blob::calculateFieldValue((BoundedGeometry *)this, &newPoint) >
        blob->threshold - INSIDE_TOLERANCE) {
        return ((int)1 - blob->inverted);
    }
    return ((int)blob->inverted);
}

void
Blob::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    Vector3Dd newPoint;
    Vector3Dd v;
    int i;
    double dist;
    double val;
    const Blob *blob = this;
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
        v = Vector3Dd(newPoint.x() - temp->getPos().x(),
            newPoint.y() - temp->getPos().y(), newPoint.z() - temp->getPos().z());
        dist = (v.x() * v.x() + v.y() * v.y() + v.z() * v.z());

        if (dist <= temp->getRadius2()) {
            val = -2.0 * (2.0 * temp->getCoeffs()[0] * dist +
                temp->getCoeffs()[1]);
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

Blob::Blob(const Blob &other) :
    Blob(other.transformation, other.transformationInverse, other.inverted,
        other.count, other.threshold, other.list, other.sturmFlag)
{
}

void *
Blob::copy()
{
    return new Blob(*this);
}

Blob *
Blob::copyWithSturmFlag(int flag) const
{
    return new Blob(transformation, transformationInverse, inverted, count,
        threshold, list, flag);
}

void
Blob::translateGeometry(Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Blob * const blob = this;
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
}

void
Blob::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Blob * const blob = this;
    if (blob->transformation == nullptr) {
        blob->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        blob->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *blob->transformation = blob->transformation->multiply(deltaTransformation);
    *blob->transformationInverse =
        deltaTransformationInverse.multiply(*blob->transformationInverse);
}

void
Blob::scaleGeometry(Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Blob * const blob = this;
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
}

void
Blob::invertGeometry()
{
    this->inverted = !this->inverted;
}

#include "java/util/PriorityQueue.txx"
