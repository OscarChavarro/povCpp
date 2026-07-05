#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/Quadric.h"

Quadric::Quadric() :
    Quadric(Vector3Dd(1.0, 1.0, 1.0), Vector3Dd(0.0, 0.0, 0.0),
        Vector3Dd(0.0, 0.0, 0.0), 1.0)
{
}

Quadric::Quadric(const Vector3Dd &object2Terms,
    const Vector3Dd &objectMixedTerms,
    const Vector3Dd &objectTerms, double objectConstant) :
    Quadric(object2Terms, objectMixedTerms, objectTerms, objectConstant,
        HUGE_VAL, false, false)
{
    updateSquareTermFlag();
}

Quadric::Quadric(const Vector3Dd &object2Terms,
    const Vector3Dd &objectMixedTerms,
    const Vector3Dd &objectTerms, double objectConstant,
    double objectVpConstant, bool constantCached, bool nonZeroSquareTerm) :
    object2Terms(object2Terms),
    objectMixedTerms(objectMixedTerms),
    objectTerms(objectTerms),
    objectConstant(objectConstant),
    objectVpConstant(objectVpConstant),
    constantCached(constantCached),
    nonZeroSquareTerm(nonZeroSquareTerm)
{
}

void
Quadric::updateSquareTermFlag()
{
    nonZeroSquareTerm =
        !((object2Terms.x() == 0.0) &&
            (object2Terms.y() == 0.0) &&
            (object2Terms.z() == 0.0) &&
            (objectMixedTerms.x() == 0.0) &&
            (objectMixedTerms.y() == 0.0) &&
            (objectMixedTerms.z() == 0.0));
}

int
Quadric::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    Quadric * const shape = this;
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;

    bool intersectionFound = false;
    if (Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        // Construct the candidate only on a real hit: ~89% of quadric tests miss
        // (drum cylinders), so deferring the ~168-byte zeroing past the test
        // removes it from the dominant miss path.
        IntersectionCandidate localElement;
        localElement.getIntersection().t = depth1;
        intersectionPoint = ray->getDirection().multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitGeometry(shape);
        localElement.getAttributes().setMaterial(materialOverride);
        depthQueue->offer(localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.getIntersection().t = depth2;
            intersectionPoint = ray->getDirection().multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitGeometry(shape);
            localElement.getAttributes().setMaterial(materialOverride);
            depthQueue->offer(localElement);
            intersectionFound = true;
        }
    }
    return (intersectionFound);
}

int
Quadric::doIntersectionForAllRayCrossingsAnnotated(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    Quadric * const shape = this;
    double depth1;
    double depth2;

    if (!Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        return false;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitGeometry(shape);
    localElement.getAttributes().setMaterial(context.materialOverride);
    localElement.getAttributes().pushDetailOwner(context.detailOwner);
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context.materialUsesObjectLocalPoint);

    localElement.getIntersection().t = depth1;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth1).add(ray->getOrigin());
    depthQueue->offer(localElement);

    if (depth2 != depth1) {
        localElement.getIntersection().t = depth2;
        localElement.getIntersection().point =
            ray->getDirection().multiply(depth2).add(ray->getOrigin());
        depthQueue->offer(localElement);
    }

    return true;
}

bool
Quadric::doIntersectionFirstHit(
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    Quadric * const shape = this;
    double depth1;
    double depth2;
    if (!Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        return false;
    }

    const double nearestDepth = depth1 < depth2 ? depth1 : depth2;
    out.getIntersection().t = nearestDepth;
    out.getIntersection().point =
        ray->getDirection().multiply(nearestDepth).add(ray->getOrigin());
    out.getAttributes().setHitGeometry(shape);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
Quadric::intersectQuadric(
    RayWithSegments *ray, Quadric *shape, double *depth1, double *depth2)
{
    double squareTerm;
    double linearTerm;
    double constantTerm;
    double tempTerm;
    double determinant;
    double determinant2;
    double a2;
    double bMinus;
    Statistics &stats = *ray->getStatistics();

    stats.getGeometryStatistics()->incrementRayQuadricTests();
    if (!ray->areQuadricConstantsCached()) {
        ray->makeRay();
    }

    if (shape->nonZeroSquareTerm) {
        squareTerm = shape->object2Terms.dotProduct(ray->getDirection2());
        tempTerm =
            shape->objectMixedTerms.dotProduct(ray->getMixedDirectionDirection());
        squareTerm += tempTerm;
    } else {
        squareTerm = 0.0;
    }

    linearTerm = shape->object2Terms.dotProduct(ray->getPositionDirection());
    linearTerm *= 2.0;
    tempTerm = shape->objectTerms.dotProduct(ray->getDirection());
    linearTerm += tempTerm;
    tempTerm =
        shape->objectMixedTerms.dotProduct(ray->getMixedPositionDirection());
    linearTerm += tempTerm;

    if (ray->isPrimaryRayEnabled()) {
        if (!shape->constantCached) {
            constantTerm = shape->object2Terms.dotProduct(ray->getPosition2());
            tempTerm = shape->objectTerms.dotProduct(ray->getOrigin());
            constantTerm += tempTerm + shape->objectConstant;
            shape->objectVpConstant = constantTerm;
            shape->constantCached = true;
        } else {
            constantTerm = shape->objectVpConstant;
        }
    } else {
        constantTerm = shape->object2Terms.dotProduct(ray->getPosition2());
        tempTerm = shape->objectTerms.dotProduct(ray->getOrigin());
        constantTerm += tempTerm + shape->objectConstant;
    }

    tempTerm = shape->objectMixedTerms.dotProduct(ray->getMixedPositionPosition());
    constantTerm += tempTerm;

    if (squareTerm != 0.0) {
        // The equation is quadratic - find its roots

        determinant2 =
            linearTerm * linearTerm - 4.0 * squareTerm * constantTerm;

        if (determinant2 < 0.0) {
            return (false);
        }

        determinant = java::Math::sqrt(determinant2);
        a2 = squareTerm * 2.0;
        bMinus = linearTerm * -1.0;

        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        // There are no quadratic terms.  Solve the linear equation instead
        if (linearTerm == 0.0) {
            return (false);
        }

        *depth1 = constantTerm * -1.0 / linearTerm;
        *depth2 = *depth1;
    }

    if ((*depth1 < Config::SMALL_TOLERANCE) || (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
            return (false);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.getGeometryStatistics()->incrementRayQuadricTestsSucceeded();
    return (true);
}

int
Quadric::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    const Quadric *shape = this;
    const double linearTerm = testPoint.dotProduct(shape->objectTerms);
    double result = linearTerm + shape->objectConstant;
    const Vector3Dd newPoint = testPoint.multiply(testPoint);
    const double squareTerm = newPoint.dotProduct(shape->object2Terms);
    result += squareTerm;
    result += shape->objectMixedTerms.x() * (testPoint.x()) * (testPoint.y()) +
              shape->objectMixedTerms.y() * (testPoint.x()) * (testPoint.z()) +
              shape->objectMixedTerms.z() * (testPoint.y()) * (testPoint.z());

    if (result < distanceTolerance) {
        return INSIDE;
    }

    return OUTSIDE;
}

void
Quadric::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    const Quadric *intersectionShape = this;
    Vector3Dd derivativeLinear;

    derivativeLinear = intersectionShape->object2Terms.multiply(2.0);
    *result = derivativeLinear.multiply(*intersectionPoint);
    *result = result->add(intersectionShape->objectTerms);

    const double nx = result->x() +
        intersectionShape->objectMixedTerms.x() * intersectionPoint->y() +
        intersectionShape->objectMixedTerms.y() * intersectionPoint->z();
    const double ny = result->y() +
        intersectionShape->objectMixedTerms.x() * intersectionPoint->x() +
        intersectionShape->objectMixedTerms.z() * intersectionPoint->z();
    const double nz = result->z() +
        intersectionShape->objectMixedTerms.y() * intersectionPoint->x() +
        intersectionShape->objectMixedTerms.z() * intersectionPoint->y();
    *result = Vector3Dd(nx, ny, nz);

    const double len = java::Math::sqrt(result->x() * result->x() +
        result->y() * result->y() + result->z() * result->z());
    if (len == 0.0) {
        // The normal is not defined at this point of the surface.  Set it
        // to any arbitrary direction
        *result = Vector3Dd(1.0, 0.0, 0.0);
    } else {
        // Normalize the normal
        *result = Vector3Dd(
            result->x() / len, result->y() / len, result->z() / len);
    }
}

void *
Quadric::copy()
{
    return new Quadric(*this);
}

AxisAlignedBoundingBox
Quadric::getMinMax() const
{
    const double a11 = object2Terms.x();
    const double a22 = object2Terms.y();
    const double a33 = object2Terms.z();
    const double a12 = objectMixedTerms.x() * 0.5;
    const double a13 = objectMixedTerms.y() * 0.5;
    const double a23 = objectMixedTerms.z() * 0.5;

    const double minor1 = a11;
    const double minor2 = a11 * a22 - a12 * a12;
    const double det =
        a11 * (a22 * a33 - a23 * a23) -
        a12 * (a12 * a33 - a23 * a13) +
        a13 * (a12 * a23 - a22 * a13);
    if (minor1 <= 1e-12 || minor2 <= 1e-12 || det <= 1e-12) {
        return AxisAlignedBoundingBox::unbounded();
    }

    const double invDet = 1.0 / det;
    const double inv11 = (a22 * a33 - a23 * a23) * invDet;
    const double inv12 = (a13 * a23 - a12 * a33) * invDet;
    const double inv13 = (a12 * a23 - a13 * a22) * invDet;
    const double inv22 = (a11 * a33 - a13 * a13) * invDet;
    const double inv23 = (a12 * a13 - a11 * a23) * invDet;
    const double inv33 = (a11 * a22 - a12 * a12) * invDet;

    const Vector3Dd linear = objectTerms;
    const Vector3Dd center(
        -0.5 * (inv11 * linear.x() + inv12 * linear.y() + inv13 * linear.z()),
        -0.5 * (inv12 * linear.x() + inv22 * linear.y() + inv23 * linear.z()),
        -0.5 * (inv13 * linear.x() + inv23 * linear.y() + inv33 * linear.z()));
    const Vector3Dd aCenter(
        a11 * center.x() + a12 * center.y() + a13 * center.z(),
        a12 * center.x() + a22 * center.y() + a23 * center.z(),
        a13 * center.x() + a23 * center.y() + a33 * center.z());
    const double radiusSquared = center.dotProduct(aCenter) - objectConstant;
    if (radiusSquared <= 1e-12 ||
        inv11 <= 0.0 || inv22 <= 0.0 || inv33 <= 0.0) {
        return AxisAlignedBoundingBox::unbounded();
    }

    const Vector3Dd extent(
        java::Math::sqrt(radiusSquared * inv11),
        java::Math::sqrt(radiusSquared * inv22),
        java::Math::sqrt(radiusSquared * inv33));
    return AxisAlignedBoundingBox{
        center.subtract(extent),
        center.add(extent)};
}

void
Quadric::invertGeometry()
{
    Quadric * const shape = this;

    shape->object2Terms = shape->object2Terms.multiply(-1.0);
    shape->objectMixedTerms = shape->objectMixedTerms.multiply(-1.0);
    shape->objectTerms = shape->objectTerms.multiply(-1.0);
    shape->objectConstant *= -1.0;
}
