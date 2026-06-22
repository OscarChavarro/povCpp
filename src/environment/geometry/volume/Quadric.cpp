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
Quadric::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    return allIntersectionsForMaterial(ray, depthQueue, nullptr);
}

int
Quadric::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *material)
{
    Quadric * const shape = this;
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    IntersectionCandidate localElement;

    bool intersectionFound = false;
    if (Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        localElement.getIntersection().t = depth1;
        intersectionPoint = ray->getDirection().multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitGeometry(shape);
        localElement.getAttributes().setMaterial(material);
        depthQueue->offer(localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.getIntersection().t = depth2;
            intersectionPoint = ray->getDirection().multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitGeometry(shape);
            localElement.getAttributes().setMaterial(material);
            depthQueue->offer(localElement);
            intersectionFound = true;
        }
    }
    return (intersectionFound);
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

    stats.incrementRayQuadricTests();
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

    stats.incrementRayQuadricTestsSucceeded();
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

void
Quadric::quadricToMatrix(const Quadric *quadric, Matrix4x4d *matrix)
{
    *matrix = Matrix4x4d::identityMatrix().multiply(0.0);
    *matrix = matrix->withVal(0, 0, quadric->object2Terms.x());
    *matrix = matrix->withVal(1, 1, quadric->object2Terms.y());
    *matrix = matrix->withVal(2, 2, quadric->object2Terms.z());
    *matrix = matrix->withVal(0, 1, quadric->objectMixedTerms.x());
    *matrix = matrix->withVal(0, 2, quadric->objectMixedTerms.y());
    *matrix = matrix->withVal(0, 3, quadric->objectTerms.x());
    *matrix = matrix->withVal(1, 2, quadric->objectMixedTerms.z());
    *matrix = matrix->withVal(1, 3, quadric->objectTerms.y());
    *matrix = matrix->withVal(2, 3, quadric->objectTerms.z());
    *matrix = matrix->withVal(3, 3, quadric->objectConstant);
}

void
Quadric::matrixToQuadric(const Matrix4x4d *matrix, Quadric *quadric)
{
    quadric->object2Terms =
        Vector3Dd(matrix->get(0, 0), matrix->get(1, 1), matrix->get(2, 2));
    quadric->objectMixedTerms = Vector3Dd(
        matrix->get(0, 1) + matrix->get(1, 0),
        matrix->get(0, 2) + matrix->get(2, 0),
        matrix->get(1, 2) + matrix->get(2, 1));
    quadric->objectTerms = Vector3Dd(
        matrix->get(0, 3) + matrix->get(3, 0),
        matrix->get(1, 3) + matrix->get(3, 1),
        matrix->get(2, 3) + matrix->get(3, 2));
    quadric->objectConstant = matrix->get(3, 3);
    quadric->updateSquareTermFlag();
}

void
Quadric::transformQuadric(Quadric *shape, const Matrix4x4d *transformationInverse)
{
    Matrix4x4d quadricMatrix;
    Matrix4x4d transformTransposed;

    Quadric::quadricToMatrix(shape, &quadricMatrix);
    quadricMatrix = transformationInverse->multiply(quadricMatrix);
    transformTransposed = transformationInverse->transpose();
    quadricMatrix = quadricMatrix.multiply(transformTransposed);
    Quadric::matrixToQuadric(&quadricMatrix, shape);
}

void
Quadric::translateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformationInverse;

    transformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    Quadric::transformQuadric(this, &transformationInverse);
}

void
Quadric::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    Quadric::transformQuadric(this, &transformationInverse);
}

void
Quadric::scaleGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformationInverse;

    transformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    Quadric::transformQuadric(this, &transformationInverse);
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
