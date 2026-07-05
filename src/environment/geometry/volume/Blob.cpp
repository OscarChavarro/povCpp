#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/QuadraticSolver.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/QuarticSolver.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/Blob.h"

constexpr double Blob::COEFFICIENT_LIMIT;
constexpr double Blob::INSIDE_TOLERANCE;
constexpr double Blob::SHADOW_ROOT_MIN_DISTANCE;

BlobElement *
Blob::allocateBlobElements(int count)
{
    if (count < 1) {
        Logger::reportMessage("Blob", Logger::FATAL_ERROR, "", "Need at least one component in a blob\n");
    }

    return new BlobElement[count];
}

Blob::Blob(double thresholdValue,
    java::ArrayList<BlobElement *> *blobElements, int numberOfPoints,
    int sturmFlagValue) :
    transformation(nullptr),
    transformationInverse(nullptr),
    inverted(false),
    count(numberOfPoints),
    threshold(thresholdValue),
    list(allocateBlobElements(numberOfPoints)),
    sturmFlag(sturmFlagValue)
{
    for (int i = 0; i < numberOfPoints; i++) {
        BlobElement *element = blobElements->get(i);
        if (java::Math::abs(element->getCoeffs()[2]) < Config::INTERSECTION_EPSILON ||
            element->getRadius2() < Config::INTERSECTION_EPSILON) {
            Logger::reportMessage(
                "Blob", Logger::ERROR, "", "Degenerate blob element\n");
        }
        double rad = element->getRadius2();
        rad *= rad;
        double coefficient = element->getCoeffs()[2];
        list[i].setRadius2(rad);
        list[i].getCoeffs()[2] = coefficient;
        list[i].getCoeffs()[1] = -(2.0 * coefficient) / rad;
        list[i].getCoeffs()[0] = coefficient / (rad * rad);
        list[i].getPos() = Vector3Dd(
            element->getPos().x(), element->getPos().y(),
            element->getPos().z());

        delete element;
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
of a particular component.  By multiplication by coefficient, we can
adjust the amount of total contribution, giving the formula:
    coefficient * (1 - (r/rad)^2)^2
This varies in strength from coefficient at r = 0, to 0 at r = rad.

Make a sorted list of points along the ray that the various blob
components start and stop adding their influence.  It would take
a very complex blob (with many components along the current ray)
to warrant the overhead of using a faster sort technique.
*/
int
Blob::determineInfluences(
    const Vector3Dd *p, const Vector3Dd *d, const Blob *blob, double minimumDistance,
    BlobInterval *intervals)
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
        if (t1 < minimumDistance) {
            t1 = 0.0;
        }
        t0 = b - disc;
        if (t0 < minimumDistance) {
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
        hit occurred at.
        */
        for (k = 0; k < cnt && t0 > intervals[k].getBound(); k++);
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
            for (k = k + 1; k < cnt && t1 > intervals[k].getBound(); k++);
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
Blob::calculateFieldValue(const Blob *blob, const Vector3Dd *pos)
{
    double density = 0.0;
    Vector3Dd v;
    BlobElement *ptr;
    ptr = &(blob->list[0]);
    for (int i = 0; i < blob->count; i++, ptr++) {
        v = ptr->getPos().subtract(*pos);
        double len = v.dotProduct(v);
        if (len < ptr->getRadius2()) {
            // Inside the radius of influence of this
            // component, add its contribution
            density +=
                len * (len * ptr->getCoeffs()[0] + ptr->getCoeffs()[1]) +
                ptr->getCoeffs()[2];
        }
    }
    return density;
}

// See if the hit in question really is a hit
bool
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
        return false;
    }
    return true;
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
Blob::traceCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    const GeometryIntersectionEmissionContext *context)
{
    Blob * const blob = this;
    IntersectionCandidate localElement;
    double dist;
    double len;
    double *tCoefficients;
    double coefficients[5];
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
    bool intersectionFound = false;
    Statistics &stats = *ray->getStatistics();

    stats.getGeometryStatistics()->incrementRayBlobTests();

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
    // component of the blob has an effect. Local, not a field on the
    // (shared, scene-wide) blob object: see BlobElement.h's comment on the
    // analogous tcoeffs scratch below for why.
    java::ArrayList<BlobInterval> localIntervals(2 * blob->count);
    if ((cnt = Blob::determineInfluences(&p, &d, blob, 0.01, localIntervals.data())) == 0) {
        // Ray doesn't hit the sphere of influence or any of
        // its component elements
        return 0;
    }
    const BlobInterval *internalIntervals = localIntervals.data();
    // Per-element scratch for the quartic-term contribution computed when a
    // component starts influencing the ray, consumed when it stops (see the
    // loop below) - local for the same reason as localIntervals above.
    java::ArrayList<double> localTCoeffs(blob->count * 5);

    // Clear out the coefficients
    for (i = 0; i < 4; i++) {
        coefficients[i] = 0.0;
    }
    coefficients[4] = -blob->threshold;

    // Step through the list of influence points, adding the
    // influence of each blob component as it appears
    for (i = 0, inFlag = 0; i < cnt; i++) {
        if (internalIntervals[i].getType() == 0) {
            // Something is just starting to influence the ray,
            // so calculate its coefficients and add them
            // into the pot.
            inFlag++;
            element = blob->list + internalIntervals[i].getIndex();

            v = p.subtract(element->getPos());
            c0 = element->getCoeffs()[0];
            c1 = element->getCoeffs()[1];
            c2 = element->getCoeffs()[2];
            t0 = v.dotProduct(v);
            t1 = v.dotProduct(d);
            tCoefficients = &localTCoeffs[internalIntervals[i].getIndex() * 5];

            tCoefficients[0] = c0;
            tCoefficients[1] = 4.0 * c0 * t1;
            tCoefficients[2] = 2.0 * c0 * (2.0 * t1 * t1 + t0) + c1;
            tCoefficients[3] = 2.0 * t1 * (2.0 * c0 * t0 + c1);
            tCoefficients[4] = c0 * t0 * t0 + c1 * t0 + c2;

            for (j = 0; j < 5; j++) {
                coefficients[j] += tCoefficients[j];
            }
        } else {
            // We are losing the influence of a component, so
            // subtract off its coefficients
            tCoefficients = &localTCoeffs[internalIntervals[i].getIndex() * 5];
            for (j = 0; j < 5; j++) {
                coefficients[j] -= tCoefficients[j];
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
            rootCount = QuarticSolver::solve(coefficients, &roots[0],
                ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                Config::POLYNOMIAL_SOLVER_EPSILON);
        } else
            // Sturm sequences
            if (java::Math::abs(coefficients[0]) < COEFFICIENT_LIMIT) {
                if (java::Math::abs(coefficients[1]) < COEFFICIENT_LIMIT) {
                    rootCount = QuadraticSolver::solve(&coefficients[2], &roots[0]);
                } else {
                    PolynomialSolver cubicSolver(3, &coefficients[1]);
                    rootCount = cubicSolver.solve(&roots[0],
                        ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                        Config::POLYNOMIAL_SOLVER_EPSILON);
                }
            } else {
                PolynomialSolver quarticSolver(4, coefficients);
                rootCount = quarticSolver.solve(&roots[0],
                    ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
                    Config::POLYNOMIAL_SOLVER_EPSILON);
            }

        // See if any of the roots are valid
        for (j = 0; j < rootCount; j++) {
            dist = roots[j];
            // First see if the root is in the interval of influence of
            // the currently active components of the blob
            if ((dist >= internalIntervals[i].getBound()) &&
                (dist <= internalIntervals[i + 1].getBound())) {
                intersectionPoint = d.multiply(dist);
                intersectionPoint = intersectionPoint.add(p);
                if (Blob::validateHit(blob, &intersectionPoint)) {
                    // Only add this hit if it really is near the surface, we
                    // can get fooled by numerical inaccuracies
                    // Transform the point into world space
                    if (blob->transformation != nullptr) {
                        intersectionPoint = blob->transformation->transformPoint(
                            intersectionPoint);
                    }
                    dv = intersectionPoint.subtract(ray->getOrigin());
                    len = dv.length();
                    localElement.getIntersection().t = len;
                    localElement.getIntersection().point = intersectionPoint;
                    localElement.getAttributes().setHitGeometry(blob);
                    localElement.getAttributes().setMaterial(
                        context != nullptr ? context->materialOverride : materialOverride);
                    if (context != nullptr) {
                        localElement.getAttributes().pushDetailOwner(context->detailOwner);
                        localElement.getAttributes().setMaterialUsesObjectLocalPoint(
                            context->materialUsesObjectLocalPoint);
                    }
                    depthQueue->offer(localElement);
                    intersectionFound = true;
                }
            }
        }
    }
    if (intersectionFound) {
        stats.getGeometryStatistics()->incrementRayBlobTestsSucceeded();
    }
    return intersectionFound;
}

int
Blob::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return traceCrossings(ray, depthQueue, materialOverride, nullptr);
}

int
Blob::doIntersectionForAllRayCrossingsAnnotated(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return traceCrossings(ray, depthQueue, context.materialOverride, &context);
}

/**
Calculate the density at this point, then compare to
the threshold to see if we are in or out of the blob
*/
int
Blob::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    (void)distanceTolerance;
    Vector3Dd newPoint;
    const Blob *blob = this;

    // Transform the point into blob space
    if (blob->transformation != nullptr) {
        newPoint = blob->transformationInverse->transformPoint(testPoint);
    } else {
        newPoint = testPoint;
    }

    // INSIDE_TOLERANCE is a density-threshold margin specific to the blob
    // field function, not the geometric point-to-surface epsilon that
    // distanceTolerance represents elsewhere; kept as-is on purpose.
    if (Blob::calculateFieldValue(blob, &newPoint) >
        blob->threshold - INSIDE_TOLERANCE) {
        return blob->inverted ? OUTSIDE : INSIDE;
    }
    return blob->inverted ? INSIDE : OUTSIDE;
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

Blob::~Blob()
{
    delete transformation;
    delete transformationInverse;
    delete[] list;
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
Blob::invertGeometry()
{
    this->inverted = !this->inverted;
}

AxisAlignedBoundingBox
Blob::getMinMax() const
{
    if (count == 0 || list == nullptr) {
        return AxisAlignedBoundingBox::unbounded();
    }
    // Build local-space AABB from all element centers +/- radius
    AxisAlignedBoundingBox local = AxisAlignedBoundingBox::empty();
    for (int i = 0; i < count; i++) {
        double r = java::Math::sqrt(list[i].getRadius2());
        const Vector3Dd &c = list[i].getPos();
        local = local.expandedBy(Vector3Dd(c.x() - r, c.y() - r, c.z() - r));
        local = local.expandedBy(Vector3Dd(c.x() + r, c.y() + r, c.z() + r));
    }
    return AxisAlignedBoundingBox::fromTransformedCorners(local.min, local.max, transformation);
}
